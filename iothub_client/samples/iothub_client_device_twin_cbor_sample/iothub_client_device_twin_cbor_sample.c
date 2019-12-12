// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample shows how to translate the Device Twin cbor received from Azure IoT Hub into meaningful data for your application.
// It uses the tinycbor library, a very lightweight cbor parser.

// WARNING: Check the return of all API calls when developing your solution. Return checks ommited for sample simplification.

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub.h"
#include "iothub_message.h"
#include "parson.h"
#include "cbor.h"
#include "cbor_helper.h"
#include <cjson/cJSON.h>

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

#define CBOR_BUFFER_SIZE 512
static uint8_t cbor_buf[CBOR_BUFFER_SIZE];

/* Paste in the your iothub device connection string  */
static const char* connectionString = "HostName=dawalton-hub.azure-devices.net;DeviceId=dane_cbor;SharedAccessKey=JmNRulx5IPuA4ax2IbyL9N9so3jr341oj0JgHIxtLcU=";
static const char const DEVICE_MODEL_NAME[] = "Contoso5000";

static const char const DEVICE_TELEMETRY_STATUS_SUCCESS[] = "success";
static const char const DEVICE_TELEMETRY_STATUS_FAILED[] = "failed";

typedef struct CONTOSO_TELEMETRY_CONFIG_TAG
{
    uint64_t frequency;
    char* status;
} CONTOSO_TELEMETRY_CONFIG;

typedef struct CONTOSO_DEVICE_TAG
{
    uint64_t battery_level;
    uint64_t device_number;
    char* device_model;
    CONTOSO_TELEMETRY_CONFIG telemetry_config;
} CONTOSO_DEVICE;


static char* serialize_to_cbor(CONTOSO_DEVICE* device)
{
    char* result;

    CborEncoder cbor_encoder_root;
    CborEncoder cbor_encoder_maker;
    CborEncoder cbor_encoder_root_container;
    cbor_encoder_init(&cbor_encoder_root, cbor_buf, CBOR_BUFFER_SIZE, 0);

    (void)cbor_encoder_create_map(&cbor_encoder_root, &cbor_encoder_root_container, 4);
        (void)cbor_encode_text_string(&cbor_encoder_root_container, "batteryLevel", sizeof("batteryLevel") - 1);
        (void)cbor_encode_uint(&cbor_encoder_root_container, device->battery_level);

        (void)cbor_encode_text_string(&cbor_encoder_root_container, "deviceNumber", sizeof("deviceNumber") - 1);
        (void)cbor_encode_uint(&cbor_encoder_root_container, device->device_number);

        (void)cbor_encode_text_string(&cbor_encoder_root_container, "deviceModel", sizeof("deviceModel") - 1);
        (void)cbor_encode_text_string(&cbor_encoder_root_container, device->device_model, strlen(device->device_model));

        (void)cbor_encode_text_string(&cbor_encoder_root_container, "telemetryConfig", sizeof("telemetryConfig") - 1);
        (void)cbor_encoder_create_map(&cbor_encoder_root_container, &cbor_encoder_maker, 2);
            (void)cbor_encode_text_string(&cbor_encoder_maker, "frequency", sizeof("frequency") - 1);
            (void)cbor_encode_uint(&cbor_encoder_maker, device->telemetry_config.frequency);
            (void)cbor_encode_text_string(&cbor_encoder_maker, "status", sizeof("status") - 1);
            (void)cbor_encode_text_string(&cbor_encoder_maker, device->telemetry_config.status, strlen(device->telemetry_config.status));
        (void)cbor_encoder_close_container(&cbor_encoder_root_container, &cbor_encoder_maker);
    (void)cbor_encoder_close_container(&cbor_encoder_root, &cbor_encoder_root_container);

    return cbor_buf;
}

static char* serialize_to_json(CONTOSO_DEVICE* device)
{
    char* result;

    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);

    //Report device props
    (void)json_object_set_number(root_object, "batteryLevel", device->battery_level);
    (void)json_object_set_number(root_object, "deviceNumber", device->device_number);
    (void)json_object_set_string(root_object, "deviceModel", device->device_model);
    (void)json_object_dotset_number(root_object, "telemetryConfig.frequency", device->telemetry_config.frequency);
    (void)json_object_dotset_string(root_object, "telemetryConfig.status", device->telemetry_config.status);

    result = json_serialize_to_string(root_value);

    return result;
}

static int find_and_copy_cbor_string(CborValue* val, const char* const find_string, char** assign_value)
{
    int return_val;

    CborValue value_out;
    size_t value_length;
    cbor_value_map_find_value(val, find_string, &value_out);
    if(cbor_value_is_valid(&value_out))
    {
        cbor_value_get_string_length(&value_out, &value_length);
        *assign_value = (char*)malloc(value_length * sizeof(char) + 1);
        cbor_value_copy_text_string(&value_out, *assign_value, &value_length, NULL);
        *(*assign_value + value_length) = '\0';
        return_val = 0;
    }
    else
    {
        *assign_value = NULL;
        return_val = 1;
    }

    return return_val;
}

static int find_and_copy_cbor_uint64(CborValue* val, const char* const find_string, uint64_t* assign_value)
{
    int return_val;

    CborValue value_out;
    cbor_value_map_find_value(val, find_string, &value_out);
    if(cbor_value_is_valid(&value_out))
    {
        cbor_value_get_uint64(&value_out, assign_value);
        return_val = 0;
    }
    else
    {
        *assign_value = 0;
        return_val = 1;
    }

    return return_val;
}

//  Converts the desired properties of the Device Twin JSON blob received from IoT Hub into a Car object.
static void parse_from_cbor(CONTOSO_DEVICE* contoso_device, const char* const cbor_string, DEVICE_TWIN_UPDATE_STATE update_state)
{
    CborParser cbor_parser;
    CborValue val;
    CborValue* root = &val;
    cbor_parser_init((const uint8_t*)cbor_string, strlen(cbor_string), 0, &cbor_parser, root);

    // Only desired properties:
    CborValue val_advance;
    CborValue returned_cbor_value;

    if (update_state == DEVICE_TWIN_UPDATE_COMPLETE)
    {
        cbor_value_map_find_value(root, "desired", &val_advance);
        root = &val_advance;
    }

    find_and_copy_cbor_uint64(root, "batteryLevel", &contoso_device->battery_level);
    find_and_copy_cbor_uint64(root, "deviceNumber", &contoso_device->device_number);
    find_and_copy_cbor_string(root, "deviceModel", &contoso_device->device_model);

    cbor_value_map_find_value(root, "telemetryConfig", &returned_cbor_value);
    if(cbor_value_is_valid(&returned_cbor_value))
    {
        find_and_copy_cbor_uint64(&returned_cbor_value, "frequency", &contoso_device->telemetry_config.frequency);
    }

}

static void device_twin_callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payload, size_t size, void* userContextCallback)
{
    (void)update_state;
    (void)size;

    CONTOSO_DEVICE* old_contoso_device = (CONTOSO_DEVICE*)userContextCallback;
    cJSON* parsed_json = cJSON_Parse(payload);
    CborEncoder cbor_encoder;
    CborEncoder cbor_encoder_container;
    cbor_encoder_init(&cbor_encoder, cbor_buf, sizeof(cbor_buf), 0);
    CborError decode_error = decode_json(parsed_json, &cbor_encoder);
    CONTOSO_DEVICE new_contoso_device;
    memset(&new_contoso_device, 0, sizeof(CONTOSO_DEVICE));
    parse_from_cbor(&new_contoso_device, cbor_buf, 0); 

    if ((&new_contoso_device)->device_number != 0)
    {
        if ((&new_contoso_device)->device_number != old_contoso_device->device_number)
        {
            printf("New device number: %" PRIu64 "\r\n", (&new_contoso_device)->device_number);
            old_contoso_device->device_number = (&new_contoso_device)->device_number;
        }
    }

    if ((&new_contoso_device)->device_model != NULL)
    {
        if (strcmp((&new_contoso_device)->device_model, old_contoso_device->device_model))
        {
            printf("New device model: %s\r\n", (&new_contoso_device)->device_model);
            if(old_contoso_device->device_model != DEVICE_MODEL_NAME)
            {
                free(old_contoso_device->device_model);
            }
            old_contoso_device->device_model = (&new_contoso_device)->device_model;
        }
    }

    if((&new_contoso_device)->telemetry_config.frequency != 0)
    {
        if ((&new_contoso_device)->telemetry_config.frequency != old_contoso_device->telemetry_config.frequency)
        {
            printf("New telemetry frequency: %" PRIu64 "\r\n", (&new_contoso_device)->telemetry_config.frequency);
            old_contoso_device->telemetry_config.frequency = (&new_contoso_device)->telemetry_config.frequency;
        }
    }
}

static void get_complete_device_twin_on_demand_callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    (void)update_state;
    (void)userContextCallback;
    printf("GetTwinAsync result:\r\n%.*s\r\n", (int)size, payLoad);
}

static void reported_state_callback(int status_code, void* userContextCallback)
{
    (void)userContextCallback;
    printf("Device Twin reported properties update completed with result: %d\r\n", status_code);
}


static void iothub_client_device_twin_and_methods_sample_run(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_DEVICE_CLIENT_HANDLE iotHubClientHandle;

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

    if (IoTHub_Init() != 0)
    {
        (void)printf("Failed to initialize the platform.\r\n");
    }
    else
    {
        if ((iotHubClientHandle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, protocol)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
            // Uncomment the following lines to enable verbose logging (e.g., for debugging).
            //bool traceOn = true;
            //(void)IoTHubDeviceClient_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &traceOn);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
            // For mbed add the certificate information
            if (IoTHubDeviceClient_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
            {
                (void)printf("failure to set option \"TrustedCerts\"\r\n");
            }
#endif // SET_TRUSTED_CERT_IN_SAMPLES

            CONTOSO_DEVICE device;
            device.battery_level = 98;
            device.device_number = 1;
            device.device_model = (char*)DEVICE_MODEL_NAME;
            device.telemetry_config.frequency = 60;
            device.telemetry_config.status = (char*)DEVICE_TELEMETRY_STATUS_SUCCESS;

            char* reported_properties_cbor = serialize_to_cbor(&device);
            char* reported_properties_json = serialize_to_json(&device);

            printf("Size of encoded cbor: %zu\r\nSize of encoded json: %zu\r\n", 
                        strlen(reported_properties_cbor), strlen(reported_properties_json));

            // (void)IoTHubDeviceClient_GetTwinAsync(iotHubClientHandle, get_complete_device_twin_on_demand_callback, NULL);
            (void)IoTHubDeviceClient_SendReportedState(iotHubClientHandle, (const unsigned char*)reported_properties_json, 
                                                            strlen(reported_properties_json), reported_state_callback, NULL);
            (void)IoTHubDeviceClient_SetDeviceTwinCallback(iotHubClientHandle, device_twin_callback, &device);

            (void)getchar();

            IoTHubDeviceClient_Destroy(iotHubClientHandle);
            // free(reportedProperties);
            if(device.device_model != DEVICE_MODEL_NAME)
            {
                free(device.device_model);
            }
        }

        IoTHub_Deinit();
    }
}

int main(void)
{
    iothub_client_device_twin_and_methods_sample_run();
    return 0;
}
