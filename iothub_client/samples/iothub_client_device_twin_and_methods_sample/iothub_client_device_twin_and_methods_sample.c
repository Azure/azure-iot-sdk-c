// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample shows how to translate the Device Twin json received from Azure IoT Hub into meaningful data for your application.
// It uses the parson library, a very lightweight json parser.

// There is an analogous sample using the serializer - which is a library provided by this SDK to help parse json - in devicetwin_simplesample.
// Most applications should use this sample, not the serializer.

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

/* Paste in the your iothub device connection string  */
static const char* connectionString = "[device connection string]";

#define DOWORK_LOOP_NUM     3

typedef struct MAKER_TAG
{
    char* makerName;
    char* style;
    int year;
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
static char* serializeToJson(Car* car)
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

//  Converts the desired properties of the Device Twin JSON blob received from IoT Hub into a Car object.
static Car* parseFromJson(const char* json, DEVICE_TWIN_UPDATE_STATE update_state)
{
    Car* car = malloc(sizeof(Car));
    JSON_Value* root_value = NULL;
    JSON_Object* root_object = NULL;

    if (NULL == car)
    {
        (void)printf("ERROR: Failed to allocate memory\r\n");
    }

    else
    {
        (void)memset(car, 0, sizeof(Car));

        root_value = json_parse_string(json);
        root_object = json_value_get_object(root_value);

        // Only desired properties:
        JSON_Value* changeOilReminder;
        JSON_Value* desired_maxSpeed;
        JSON_Value* latitude;
        JSON_Value* longitude;

        if (update_state == DEVICE_TWIN_UPDATE_COMPLETE)
        {
            changeOilReminder = json_object_dotget_value(root_object, "desired.changeOilReminder");
            desired_maxSpeed = json_object_dotget_value(root_object, "desired.settings.desired_maxSpeed");
            latitude = json_object_dotget_value(root_object, "desired.settings.location.latitude");
            longitude = json_object_dotget_value(root_object, "desired.settings.location.longitude");
        }
        else
        {
            changeOilReminder = json_object_dotget_value(root_object, "changeOilReminder");
            desired_maxSpeed = json_object_dotget_value(root_object, "settings.desired_maxSpeed");
            latitude = json_object_dotget_value(root_object, "settings.location.latitude");
            longitude = json_object_dotget_value(root_object, "settings.location.longitude");
        }

        if (changeOilReminder != NULL)
        {
            const char* data = json_value_get_string(changeOilReminder);

            if (data != NULL)
            {
                car->changeOilReminder = malloc(strlen(data) + 1);
                if (NULL != car->changeOilReminder)
                {
                    (void)strcpy(car->changeOilReminder, data);
                }
            }
        }

        if (desired_maxSpeed != NULL)
        {
            car->settings.desired_maxSpeed = (uint8_t)json_value_get_number(desired_maxSpeed);
        }

        if (latitude != NULL)
        {
            car->settings.location.latitude = json_value_get_number(latitude);
        }

        if (longitude != NULL)
        {
            car->settings.location.longitude = json_value_get_number(longitude);
        }
        json_value_free(root_value);
    }

    return car;
}

static int deviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
    (void)userContextCallback;
    (void)payload;
    (void)size;

    int result;

    if (strcmp("getCarVIN", method_name) == 0)
    {
        const char deviceMethodResponse[] = "{ \"Response\": \"1HGCM82633A004352\" }";
        *response_size = sizeof(deviceMethodResponse)-1;
        *response = malloc(*response_size);
        (void)memcpy(*response, deviceMethodResponse, *response_size);
        result = 200;
    }
    else
    {
        // All other entries are ignored.
        const char deviceMethodResponse[] = "{ }";
        *response_size = sizeof(deviceMethodResponse)-1;
        *response = malloc(*response_size);
        (void)memcpy(*response, deviceMethodResponse, *response_size);
        result = -1;
    }

    return result;
}

static void getCompleteDeviceTwinOnDemandCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    (void)update_state;
    (void)userContextCallback;
    printf("GetTwinAsync result:\r\n%.*s\r\n", (int)size, payLoad);
}

static void deviceTwinCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    (void)update_state;
    (void)size;

    Car* oldCar = (Car*)userContextCallback;
    Car* newCar = parseFromJson((const char*)payLoad, update_state);

    if (NULL == newCar)
    {
        printf("ERROR: parseFromJson returned NULL\r\n");
    }
    else
    {
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
}

static void reportedStateCallback(int status_code, void* userContextCallback)
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

            Car car;
            memset(&car, 0, sizeof(Car));
            car.lastOilChangeDate = "2016";
            car.maker.makerName = "Fabrikam";
            car.maker.style = "sedan";
            car.maker.year = 2014;
            car.state.reported_maxSpeed = 100;
            car.state.softwareVersion = 1;
            car.state.vanityPlate = "1I1";

            char* reportedProperties = serializeToJson(&car);

            (void)IoTHubDeviceClient_GetTwinAsync(iotHubClientHandle, getCompleteDeviceTwinOnDemandCallback, NULL);
            (void)IoTHubDeviceClient_SendReportedState(iotHubClientHandle, (const unsigned char*)reportedProperties, strlen(reportedProperties), reportedStateCallback, NULL);
            (void)IoTHubDeviceClient_SetDeviceMethodCallback(iotHubClientHandle, deviceMethodCallback, NULL);
            (void)IoTHubDeviceClient_SetDeviceTwinCallback(iotHubClientHandle, deviceTwinCallback, &car);

            (void)getchar();

            IoTHubDeviceClient_Destroy(iotHubClientHandle);
            free(reportedProperties);
            free(car.changeOilReminder);
        }

        IoTHub_Deinit();
    }
}

int main(void)
{
    iothub_client_device_twin_and_methods_sample_run();
    return 0;
}
