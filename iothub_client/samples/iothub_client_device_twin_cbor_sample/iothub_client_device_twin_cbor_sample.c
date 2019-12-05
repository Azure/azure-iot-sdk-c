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

#define CBOR_BUFFER_SIZE 200
static uint8_t cbor_buf[CBOR_BUFFER_SIZE];

/* Paste in the your iothub device connection string  */
static const char* connectionString = "HostName=dawalton-hub.azure-devices.net;DeviceId=dawalton-test2;SharedAccessKey=45vmWGYHdtOHdD+cipw5yR/GtmJ4h9Z07ipHwu4WnPQ=";
static uint8_t test_cbor_value[] = { 0xd8, 0x20, 0x76, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 
                                0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d }; // http://example.com

/*
{
    "lastOilChangeDate": "2016",
    "maker": {
        "makerName": "Fabrikam",
        "style": "sedan",
        "year": 2014
    },
    "state": {
        "reported_maxSpeed": 100,
        "softwareVersion": 1,
        "vanityPlate": "1I1"
    }
}
*/
static uint8_t test_cbor_twin[] = {0xA1, 0x67, 0x64, 0x65, 0x73, 0x69, 0x72, 0x65, 0x64, 0xA4, 0x71, 0x6C, 0x61, 0x73, 
                            0x74, 0x4F, 0x69, 0x6C, 0x43, 0x68, 0x61, 0x6E, 0x67, 0x65, 0x44, 0x61, 0x74, 0x65, 0x64, 0x32, 0x30, 
                            0x31, 0x38, 0x71, 0x63, 0x68, 0x61, 0x6E, 0x67, 0x65, 0x4F, 0x69, 0x6C, 0x52, 0x65, 0x6D, 0x69, 0x6E, 
                            0x64, 0x65, 0x72, 0x64, 0x32, 0x30, 0x31, 0x37, 0x69, 0x24, 0x6D, 0x65, 0x74, 0x61, 0x64, 0x61, 0x74, 
                            0x61, 0xA4, 0x6C, 0x24, 0x6C, 0x61, 0x73, 0x74, 0x55, 0x70, 0x64, 0x61, 0x74, 0x65, 0x64, 0x78, 0x1C, 
                            0x32, 0x30, 0x31, 0x39, 0x2D, 0x31, 0x32, 0x2D, 0x30, 0x33, 0x54, 0x32, 0x33, 0x3A, 0x35, 0x33, 0x3A, 
                            0x35, 0x38, 0x2E, 0x34, 0x39, 0x38, 0x36, 0x30, 0x32, 0x39, 0x5A, 0x73, 0x24, 0x6C, 0x61, 0x73, 0x74, 
                            0x55, 0x70, 0x64, 0x61, 0x74, 0x65, 0x64, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x0B, 0x71, 0x6C, 
                            0x61, 0x73, 0x74, 0x4F, 0x69, 0x6C, 0x43, 0x68, 0x61, 0x6E, 0x67, 0x65, 0x44, 0x61, 0x74, 0x65, 0xA2, 
                            0x6C, 0x24, 0x6C, 0x61, 0x73, 0x74, 0x55, 0x70, 0x64, 0x61, 0x74, 0x65, 0x64, 0x78, 0x1C, 0x32, 0x30, 
                            0x31, 0x39, 0x2D, 0x31, 0x32, 0x2D, 0x30, 0x33, 0x54, 0x32, 0x33, 0x3A, 0x35, 0x33, 0x3A, 0x35, 0x38, 
                            0x2E, 0x34, 0x39, 0x38, 0x36, 0x30, 0x32, 0x39, 0x5A, 0x73, 0x24, 0x6C, 0x61, 0x73, 0x74, 0x55, 0x70, 
                            0x64, 0x61, 0x74, 0x65, 0x64, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x0B, 0x71, 0x63, 0x68, 0x61, 
                            0x6E, 0x67, 0x65, 0x4F, 0x69, 0x6C, 0x52, 0x65, 0x6D, 0x69, 0x6E, 0x64, 0x65, 0x72, 0xA2, 0x6C, 0x24, 
                            0x6C, 0x61, 0x73, 0x74, 0x55, 0x70, 0x64, 0x61, 0x74, 0x65, 0x64, 0x78, 0x1C, 0x32, 0x30, 0x31, 0x39, 
                            0x2D, 0x31, 0x32, 0x2D, 0x30, 0x33, 0x54, 0x32, 0x33, 0x3A, 0x35, 0x33, 0x3A, 0x35, 0x38, 0x2E, 0x34, 
                            0x39, 0x38, 0x36, 0x30, 0x32, 0x39, 0x5A, 0x73, 0x24, 0x6C, 0x61, 0x73, 0x74, 0x55, 0x70, 0x64, 0x61, 
                            0x74, 0x65, 0x64, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x0B, 0x68, 0x24, 0x76, 0x65, 0x72, 0x73, 
                            0x69, 0x6F, 0x6E, 0x0B};

CborParser cbor_parser;

typedef struct MAKER_TAG
{
    char* makerName;
    char* style;
    uint64_t year;
} Maker;

typedef struct GEO_TAG
{
    double longitude;
    double latitude;
} Geo;

typedef struct CAR_STATE_TAG
{
    int32_t softwareVersion;        // reported property
    uint8_t reported_maxSpeed;      // reported property
    char* vanityPlate;              // reported property
} CarState;

typedef struct CAR_SETTINGS_TAG
{
    uint8_t desired_maxSpeed;       // desired property
    Geo location;                   // desired property
} CarSettings;

typedef struct CAR_TAG
{
    char* lastOilChangeDate;        // reported property
    char* changeOilReminder;        // desired property
    Maker maker;                    // reported property
    CarState state;                 // reported property
    CarSettings settings;           // desired property
} Car;

//  Converts the Car object into a JSON blob with reported properties that is ready to be sent across the wire as a twin.
static char* serialize_to_json(Car* car)
{
    char* result;

    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);

    // Only reported properties:
    (void)json_object_set_string(root_object, "lastOilChangeDate", car->lastOilChangeDate);
    (void)json_object_dotset_string(root_object, "maker.makerName", car->maker.makerName);
    (void)json_object_dotset_string(root_object, "maker.style", car->maker.style);
    (void)json_object_dotset_number(root_object, "maker.year", car->maker.year);
    (void)json_object_dotset_number(root_object, "state.reported_maxSpeed", car->state.reported_maxSpeed);
    (void)json_object_dotset_number(root_object, "state.softwareVersion", car->state.softwareVersion);
    (void)json_object_dotset_string(root_object, "state.vanityPlate", car->state.vanityPlate);

    result = json_serialize_to_string(root_value);

    json_value_free(root_value);

    return result;
}

static char* serialize_to_cbor(Car* car)
{
    char* result;

    CborEncoder cbor_encoder_root;
    CborEncoder cbor_encoder_maker;
    CborEncoder cbor_encoder_root_container;
    cbor_encoder_init(&cbor_encoder_root, cbor_buf, CBOR_BUFFER_SIZE, 0);

    CborError cbor_error;
    cbor_error = cbor_encoder_create_map(&cbor_encoder_root, &cbor_encoder_root_container, 3);
    cbor_error = cbor_encode_text_string(&cbor_encoder_root_container, "lastOilChangeDate", sizeof("lastOilChangeDate") - 1);
    cbor_error = cbor_encode_text_string(&cbor_encoder_root_container, car->lastOilChangeDate, strlen(car->lastOilChangeDate));
    cbor_error = cbor_encode_text_string(&cbor_encoder_root_container, "maker", sizeof("maker") - 1);
    cbor_error = cbor_encoder_create_map(&cbor_encoder_root_container, &cbor_encoder_maker, 3);
    cbor_error = cbor_encode_text_string(&cbor_encoder_maker, "makerName", sizeof("makerName") - 1);
    cbor_error = cbor_encode_text_string(&cbor_encoder_maker, car->maker.makerName, strlen(car->maker.makerName));
    cbor_error = cbor_encode_text_string(&cbor_encoder_maker, "style", sizeof("style") - 1);
    cbor_error = cbor_encode_text_string(&cbor_encoder_maker, car->maker.style, strlen(car->maker.style));
    cbor_error = cbor_encode_text_string(&cbor_encoder_maker, "year", sizeof("year") - 1);
    cbor_error = cbor_encode_uint(&cbor_encoder_maker, car->maker.year);
    cbor_error = cbor_encoder_close_container(&cbor_encoder_root_container, &cbor_encoder_maker);
    cbor_error = cbor_encode_text_string(&cbor_encoder_root_container, "state", sizeof("state") - 1);
    cbor_error = cbor_encoder_create_map(&cbor_encoder_root_container, &cbor_encoder_maker, 3);
    cbor_error = cbor_encode_text_string(&cbor_encoder_maker, "reported_maxSpeed", sizeof("reported_maxSpeed") - 1);
    cbor_error = cbor_encode_uint(&cbor_encoder_maker, car->state.reported_maxSpeed);
    cbor_error = cbor_encode_text_string(&cbor_encoder_maker, "softwareVersion", sizeof("softwareVersion") - 1);
    cbor_error = cbor_encode_uint(&cbor_encoder_maker, car->state.softwareVersion);
    cbor_error = cbor_encode_text_string(&cbor_encoder_maker, "vanityPlate", sizeof("vanityPlate") - 1);
    cbor_error = cbor_encode_text_string(&cbor_encoder_maker, car->state.vanityPlate, strlen(car->state.vanityPlate));
    cbor_error = cbor_encoder_close_container(&cbor_encoder_root_container, &cbor_encoder_maker);
    cbor_error = cbor_encoder_close_container(&cbor_encoder_root, &cbor_encoder_root_container);

    return cbor_buf;
}

static void dumpbytes(const uint8_t *buf, size_t len)
{
    while (len--)
        printf("%02X ", *buf++);
}

static void indent(int nestingLevel)
{
    while (nestingLevel--)
        puts("  ");
}

static CborError cbor_parse_recursive(CborValue *it, int nesting_level)
{
    while (!cbor_value_at_end(it)) {
        CborError err;
        CborType type = cbor_value_get_type(it);

        indent(nesting_level);
        switch (type) {
        case CborArrayType:
        case CborMapType: {
            // recursive type
            CborValue recursed;
            assert(cbor_value_is_container(it));
            puts(type == CborArrayType ? "Array[" : "Map[");
            err = cbor_value_enter_container(it, &recursed);
            if (err)
                return err;       // parse error
            err = cbor_parse_recursive(&recursed, nesting_level + 1);
            if (err)
                return err;       // parse error
            err = cbor_value_leave_container(it, &recursed);
            if (err)
                return err;       // parse error
            indent(nesting_level);
            puts("]");
            continue;
        }

        case CborIntegerType: {
            int64_t val;
            cbor_value_get_int64(it, &val);     // can't fail
            printf("%lld\n", (long long)val);
            break;
        }

        case CborByteStringType: {
            uint8_t *buf;
            size_t n;
            err = cbor_value_dup_byte_string(it, &buf, &n, it);
            if (err)
                return err;     // parse error
            dumpbytes(buf, n);
            puts("");
            free(buf);
            continue;
        }

        case CborTextStringType: {
            char *buf;
            size_t n;
            err = cbor_value_dup_text_string(it, &buf, &n, it);
            if (err)
                return err;     // parse error
            puts(buf);
            free(buf);
            continue;
        }

        case CborTagType: {
            CborTag tag;
            cbor_value_get_tag(it, &tag);       // can't fail
            printf("Tag(%lld)\n", (long long)tag);
            break;
        }

        case CborSimpleType: {
            uint8_t type;
            cbor_value_get_simple_type(it, &type);  // can't fail
            printf("simple(%u)\n", type);
            break;
        }

        case CborNullType:
            puts("null");
            break;

        case CborUndefinedType:
            puts("undefined");
            break;

        case CborBooleanType: {
            bool val;
            cbor_value_get_boolean(it, &val);       // can't fail
            puts(val ? "true" : "false");
            break;
        }

        case CborDoubleType: {
            double val;
            if (false) {
                float f;
        case CborFloatType:
                cbor_value_get_float(it, &f);
                val = f;
            } else {
                cbor_value_get_double(it, &val);
            }
            printf("%g\n", val);
            break;
        }
        case CborHalfFloatType: {
            uint16_t val;
            cbor_value_get_half_float(it, &val);
            printf("__f16(%04x)\n", val);
            break;
        }

        case CborInvalidType:
            assert(false);      // can't happen
            break;
        }

        err = cbor_value_advance_fixed(it);
        if (err)
            return err;
    }
    return CborNoError;
}

//  Converts the desired properties of the Device Twin JSON blob received from IoT Hub into a Car object.
static Car* parse_from_cbor(Car* car, CborValue* val, CborEncoder* cbor_encoder, DEVICE_TWIN_UPDATE_STATE update_state)
{

    // Only desired properties:
    // JSON_Value* changeOilReminder;
    // JSON_Value* desired_maxSpeed;
    // JSON_Value* latitude;
    // JSON_Value* longitude;

    if (update_state == DEVICE_TWIN_UPDATE_COMPLETE)
    {
        // changeOilReminder = json_object_dotget_value(root_object, "desired.changeOilReminder");
        // desired_maxSpeed = json_object_dotget_value(root_object, "desired.settings.desired_maxSpeed");
        // latitude = json_object_dotget_value(root_object, "desired.settings.location.latitude");
        // longitude = json_object_dotget_value(root_object, "desired.settings.location.longitude");
    }
    else
    {
        // changeOilReminder = json_object_dotget_value(root_object, "changeOilReminder");
        // desired_maxSpeed = json_object_dotget_value(root_object, "settings.desired_maxSpeed");
        // latitude = json_object_dotget_value(root_object, "settings.location.latitude");
        // longitude = json_object_dotget_value(root_object, "settings.location.longitude");
    }

    // if (changeOilReminder != NULL)
    // {
    //     const char* data = json_value_get_string(changeOilReminder);

    //     if (data != NULL)
    //     {
    //         car->changeOilReminder = malloc(strlen(data) + 1);
    //         if (NULL != car->changeOilReminder)
    //         {
    //             (void)strcpy(car->changeOilReminder, data);
    //         }
    //     }
    // }

    // if (desired_maxSpeed != NULL)
    // {
    //     car->settings.desired_maxSpeed = (uint8_t)json_value_get_number(desired_maxSpeed);
    // }

    // if (latitude != NULL)
    // {
    //     car->settings.location.latitude = json_value_get_number(latitude);
    // }

    // if (longitude != NULL)
    // {
    //     car->settings.location.longitude = json_value_get_number(longitude);
    // }

    return car;
}

static Car* parse_from_json(const char* json, DEVICE_TWIN_UPDATE_STATE update_state)
{
    //Convert to CBOR
    CborEncoder cbor_encoder;
    // decode_json(json, &cbor_encoder);
    CborValue cbor_val;
    // cbor_parser_init((const uint8_t*)cbor, 100, 0, &cbor_parser, &cbor_val);

    CborValue root;
    Car* car = malloc(sizeof(Car));
    if (car != NULL)
    {
        // parse_from_cbor(car, root);
    }
}

static void device_twin_callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    (void)update_state;
    (void)size;

    Car* oldCar = (Car*)userContextCallback;
    Car* newCar = parse_from_json((const char*)payLoad, update_state);

    if (newCar->changeOilReminder != NULL)
    {
        if ((oldCar->changeOilReminder != NULL) && (strcmp(oldCar->changeOilReminder, newCar->changeOilReminder) != 0))
        {
            free(oldCar->changeOilReminder);
        }
        
        if (oldCar->changeOilReminder == NULL)
        {
            printf("Received a new changeOilReminder = %s\n", newCar->changeOilReminder);
            if ( NULL != (oldCar->changeOilReminder = malloc(strlen(newCar->changeOilReminder) + 1)))
            {
                (void)strcpy(oldCar->changeOilReminder, newCar->changeOilReminder);
                free(newCar->changeOilReminder);
            }
        }
    }

    if (newCar->settings.desired_maxSpeed != 0)
    {
        if (newCar->settings.desired_maxSpeed != oldCar->settings.desired_maxSpeed)
        {
            printf("Received a new desired_maxSpeed = %" PRIu8 "\n", newCar->settings.desired_maxSpeed);
            oldCar->settings.desired_maxSpeed = newCar->settings.desired_maxSpeed;
        }
    }

    if (newCar->settings.location.latitude != 0)
    {
        if (newCar->settings.location.latitude != oldCar->settings.location.latitude)
        {
            printf("Received a new latitude = %f\n", newCar->settings.location.latitude);
            oldCar->settings.location.latitude = newCar->settings.location.latitude;
        }
    }

    if (newCar->settings.location.longitude != 0)
    {
        if (newCar->settings.location.longitude != oldCar->settings.location.longitude)
        {
            printf("Received a new longitude = %f\n", newCar->settings.location.longitude);
            oldCar->settings.location.longitude = newCar->settings.location.longitude;
        }
    }

    free(newCar);
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
//     IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
//     IOTHUB_DEVICE_CLIENT_HANDLE iotHubClientHandle;

//     // Select the Protocol to use with the connection
// #ifdef SAMPLE_MQTT
//     protocol = MQTT_Protocol;
// #endif // SAMPLE_MQTT
// #ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
//     protocol = MQTT_WebSocket_Protocol;
// #endif // SAMPLE_MQTT_OVER_WEBSOCKETS
// #ifdef SAMPLE_AMQP
//     protocol = AMQP_Protocol;
// #endif // SAMPLE_AMQP
// #ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
//     protocol = AMQP_Protocol_over_WebSocketsTls;
// #endif // SAMPLE_AMQP_OVER_WEBSOCKETS
// #ifdef SAMPLE_HTTP
//     protocol = HTTP_Protocol;
// #endif // SAMPLE_HTTP

//     if (IoTHub_Init() != 0)
//     {
//         (void)printf("Failed to initialize the platform.\r\n");
//     }
//     else
//     {
//         if ((iotHubClientHandle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, protocol)) == NULL)
//         {
//             (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
//         }
//         else
//         {
//             // Uncomment the following lines to enable verbose logging (e.g., for debugging).
//             //bool traceOn = true;
//             //(void)IoTHubDeviceClient_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &traceOn);

// #ifdef SET_TRUSTED_CERT_IN_SAMPLES
//             // For mbed add the certificate information
//             if (IoTHubDeviceClient_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
//             {
//                 (void)printf("failure to set option \"TrustedCerts\"\r\n");
//             }
// #endif // SET_TRUSTED_CERT_IN_SAMPLES

            Car car;
            memset(&car, 0, sizeof(Car));
            car.lastOilChangeDate = "2016";
            car.maker.makerName = "Fabrikam";
            car.maker.style = "sedan";
            car.maker.year = 2014;
            car.state.reported_maxSpeed = 100;
            car.state.softwareVersion = 1;
            car.state.vanityPlate = "1I1";

            // char* reportedProperties = serialize_to_json(&car);
            char* reported_properties_cbor = serialize_to_cbor(&car);

            CborParser cbor_parser;
            CborValue cbor_value;
            cbor_parser_init((const uint8_t*)reported_properties_cbor, strlen(reported_properties_cbor), 0, &cbor_parser, &cbor_value);
            cbor_parse_recursive(&cbor_value, 0);

            // (void)IoTHubDeviceClient_GetTwinAsync(iotHubClientHandle, get_complete_device_twin_on_demand_callback, NULL);
            // (void)IoTHubDeviceClient_SendReportedState(iotHubClientHandle, (const unsigned char*)reportedProperties, strlen(reportedProperties), reported_state_callback, NULL);
            // (void)IoTHubDeviceClient_SetDeviceTwinCallback(iotHubClientHandle, device_twin_callback, &car);

            (void)getchar();

            // IoTHubDeviceClient_Destroy(iotHubClientHandle);
            // free(reportedProperties);
            free(car.changeOilReminder);
        // }

        // IoTHub_Deinit();
    // }
}

int main(void)
{
    iothub_client_device_twin_and_methods_sample_run();
    return 0;
}
