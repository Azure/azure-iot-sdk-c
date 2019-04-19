// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//
// IMPORTANT: Please read and understand serializer limitations and alternatives as
//            described ../../readme.md before beginning to use the serializer.
//

// There is an analogous sample using the iothub_client and parson (a json-parsing library):
// <azure-iot-sdk-c repo>/iothub_client/samples/iothub_client_device_twin_and_methods_sample

// Most applications should use that sample, not the serializer.

#include <stdio.h>
#include <inttypes.h>

#include "serializer_devicetwin.h"
#include "iothub_client.h"
#include "iothubtransportamqp.h"
#include "iothubtransportmqtt.h"
#include "azure_c_shared_utility/threadapi.h"
#include "parson.h"
#include "azure_c_shared_utility/platform.h"

/*String containing Hostname, Device Id & Device Key in the format:             */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"    */
static const char* connectionString = "HostName=...";

// Define the Model - it is a car.
BEGIN_NAMESPACE(Contoso);

DECLARE_STRUCT(Maker,
    ascii_char_ptr, makerName, /*Fabrikam, Contoso ... */
    ascii_char_ptr, style, /*sedan, minivan ...*/
    int, year
);

DECLARE_STRUCT(Geo,
    double, longitude,
    double, latitude
);

DECLARE_MODEL(CarState,
    WITH_REPORTED_PROPERTY(int32_t, softwareVersion),
    WITH_REPORTED_PROPERTY(uint8_t, reported_maxSpeed),
    WITH_REPORTED_PROPERTY(ascii_char_ptr, vanityPlate)
);

// NOTE: For callbacks defined in the serializer model to be fired for desired properties, IoTHubClient_SetDeviceTwinCallback must not be invoked.
//       Please comment out the call to IoTHubClient_SetDeviceTwinCallback further down to enable the callbacks defined in the model below.
DECLARE_MODEL(CarSettings,
    WITH_DESIRED_PROPERTY(uint8_t, desired_maxSpeed, onDesiredMaxSpeed),
    WITH_DESIRED_PROPERTY(Geo, location)
);

DECLARE_DEVICETWIN_MODEL(Car,
    WITH_REPORTED_PROPERTY(ascii_char_ptr, lastOilChangeDate), /*this is a simple reported property*/
    WITH_DESIRED_PROPERTY(ascii_char_ptr, changeOilReminder),

    WITH_REPORTED_PROPERTY(Maker, maker), /*this is a structured reported property*/
    WITH_REPORTED_PROPERTY(CarState, state), /*this is a model in model*/
    WITH_DESIRED_PROPERTY(CarSettings, settings), /*this is a model in model*/
    WITH_METHOD(getCarVIN)
);

END_NAMESPACE(Contoso);

METHODRETURN_HANDLE getCarVIN(Car* car)
{
    (void)(car);
    /*Car VINs are JSON strings, for example: 1HGCM82633A004352*/
    METHODRETURN_HANDLE result = MethodReturn_Create(201, "\"1HGCM82633A004352\"");
    return result;
}

void deviceTwinReportStateCallback(int status_code, void* userContextCallback)
{
    (void)(userContextCallback);
    printf("IoTHub: reported properties delivered with status_code = %d\n", status_code);
}

static void deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    (void)userContextCallback;
    printf("Device Twin properties received: update=%s payload=%s, size=%lu\n", MU_ENUM_TO_STRING(DEVICE_TWIN_UPDATE_STATE, update_state), payLoad, (unsigned long)size);
}

void onDesiredMaxSpeed(void* argument)
{
    // Note: The argument is NOT a pointer to desired_maxSpeed, but instead a pointer to the MODEL
    //       that contains desired_maxSpeed as one of its arguments.  In this case, it
    //       is CarSettings*.

    CarSettings* car = argument;
    printf("received a new desired_maxSpeed = %" PRIu8 "\n", car->desired_maxSpeed);
}

void device_twin_simple_sample_run(void)
{
    /*prepare the platform*/
    if (platform_init() != 0)
    {
        printf("Failed to initialize the platform.\n");
    }
    else
    {
        if (SERIALIZER_REGISTER_NAMESPACE(Contoso) == NULL)
        {
            LogError("unable to SERIALIZER_REGISTER_NAMESPACE");
        }
        else
        {
            /*create an IoTHub client*/
            IOTHUB_CLIENT_HANDLE iotHubClientHandle = IoTHubClient_CreateFromConnectionString(connectionString, MQTT_Protocol); // Change to AMQP_Procotol if desired.
            if (iotHubClientHandle == NULL)
            {
                printf("Failure creating IoTHubClient handle");
            }
            else
            {
                // Turn on Log
                bool trace = true;
                (void)IoTHubClient_SetOption(iotHubClientHandle, "logtrace", &trace);

                Car* car = IoTHubDeviceTwin_CreateCar(iotHubClientHandle);
                if (car == NULL)
                {
                    printf("Failure in IoTHubDeviceTwin_CreateCar");
                }
                else
                {
                    /*setting values for reported properties*/
                    car->lastOilChangeDate = "2016";
                    car->maker.makerName = "Fabrikam";
                    car->maker.style = "sedan";
                    car->maker.year = 2014;
                    car->state.reported_maxSpeed = 100;
                    car->state.softwareVersion = 1;
                    car->state.vanityPlate = "1I1";

                    // IoTHubDeviceTwin_SendReportedStateCar sends the reported status back to IoT Hub
                    // to the associated device twin.
                    //
                    // IoTHubDeviceTwin_SendReportedStateCar is an auto-generated function, created
                    // via the macro DECLARE_DEVICETWIN_MODEL(Car,...).  It resolves to the underlying function
                    // IoTHubDeviceTwin_SendReportedState_Impl().
                    if (IoTHubDeviceTwin_SendReportedStateCar(car, deviceTwinReportStateCallback, NULL) != IOTHUB_CLIENT_OK)
                    {
                        (void)printf("Failed sending serialized reported state\n");
                    }
                    else
                    {
                        printf("Reported state will be send to IoTHub\n");

                        // Comment out the following three lines if you want to enable callback(s) for updates of the existing model (example: onDesiredMaxSpeed)
                        if (IoTHubClient_SetDeviceTwinCallback(iotHubClientHandle, deviceTwinGetStateCallback, NULL) != IOTHUB_CLIENT_OK)
                        {
                            (void)printf("Failed subscribing for device twin properties\n");
                        }
                    }

                    printf("press ENTER to end the sample\n");
                    (void)getchar();

                }
                IoTHubDeviceTwin_DestroyCar(car);
            }
            IoTHubClient_Destroy(iotHubClientHandle);
        }
    }
    platform_deinit();
}

int main(void)
{
    device_twin_simple_sample_run();
    return 0;
}

