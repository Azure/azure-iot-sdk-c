// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//
// IMPORTANT: Please read and understand serializer limitations and alternatives as
//            described ../../readme.md before beginning to use the serializer.
//

#include <stdlib.h>

#include <stdio.h>
#include <stdint.h>

/* This sample uses the _LL APIs of iothub_client for example purposes.
That does not mean that MQTT only works with the _LL APIs.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "serializer.h"
#include "iothub_client_ll.h"
#include "iothubtransportmqtt.h"

#ifdef MBED_BUILD_TIMESTAMP
#define SET_TRUSTED_CERT_IN_SAMPLES
#endif // MBED_BUILD_TIMESTAMP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES


/*String containing Hostname, Device Id & Device Key in the format:             */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"    */
static const char* connectionString = "[device connection string]";

// Define the Model
BEGIN_NAMESPACE(WeatherStation);

DECLARE_MODEL(ContosoAnemometer,
WITH_DATA(ascii_char_ptr, DeviceId),
WITH_DATA(int, WindSpeed),
WITH_DATA(float, Temperature),
WITH_DATA(float, Humidity),
WITH_ACTION(TurnFanOn),
WITH_ACTION(TurnFanOff),
WITH_ACTION(SetAirResistance, int, Position)
);

END_NAMESPACE(WeatherStation);

static char propText[1024];

EXECUTE_COMMAND_RESULT TurnFanOn(ContosoAnemometer* device)
{
    (void)device;
    (void)printf("Turning fan on.\r\n");
    return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT TurnFanOff(ContosoAnemometer* device)
{
    (void)device;
    (void)printf("Turning fan off.\r\n");
    return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT SetAirResistance(ContosoAnemometer* device, int Position)
{
    (void)device;
    (void)printf("Setting Air Resistance Position to %d.\r\n", Position);
    return EXECUTE_COMMAND_SUCCESS;
}

void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    unsigned int messageTrackingId = (unsigned int)(uintptr_t)userContextCallback;

    (void)printf("Message Id: %u Received.\r\n", messageTrackingId);

    (void)printf("Result Call Back Called! Result is: %s \r\n", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size, ContosoAnemometer *myWeather)
{
    static unsigned int messageTrackingId;
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
    if (messageHandle == NULL)
    {
        printf("unable to create a new IoTHubMessage\r\n");
    }
    else
    {
        MAP_HANDLE propMap = IoTHubMessage_Properties(messageHandle);
        (void)sprintf_s(propText, sizeof(propText), myWeather->Temperature > 28 ? "true" : "false");
        if (propMap == NULL || Map_AddOrUpdate(propMap, "temperatureAlert", propText) != MAP_OK)
        {
            (void)printf("ERROR: Map_AddOrUpdate Failed!\r\n");
        }

        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)(uintptr_t)messageTrackingId) != IOTHUB_CLIENT_OK)
        {
            printf("failed to hand over the message to IoTHubClient");
        }
        else
        {
            printf("IoTHubClient accepted the message for delivery\r\n");
        }
        IoTHubMessage_Destroy(messageHandle);
    }
    messageTrackingId++;
}

/*this function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    const unsigned char* buffer;
    size_t size;
    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        (void)printf("unable to IoTHubMessage_GetByteArray\r\n");
    }
    else
    {
        /*buffer is not zero terminated*/
        char* temp = malloc(size + 1);
        if (temp == NULL)
        {
            (void)printf("failed to malloc\r\n");
        }
        else
        {
            (void)memcpy(temp, buffer, size);
            temp[size] = '\0';
            EXECUTE_COMMAND_RESULT executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
            if (executeCommandResult != EXECUTE_COMMAND_SUCCESS)
            {
                (void)printf("Execute command failed\r\n");
            }
            free(temp);
        }
    }
    // MQTT can only accept messages
    return IOTHUBMESSAGE_ACCEPTED;
}

void simplesample_mqtt_run(void)
{
    if (platform_init() != 0)
    {
        (void)printf("Failed to initialize platform.\r\n");
    }
    else
    {
        if (serializer_init(NULL) != SERIALIZER_OK)
        {
            (void)printf("Failed on serializer_init\r\n");
        }
        else
        {
            IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
            srand((unsigned int)time(NULL));
            int avgWindSpeed = 10;
            float minTemperature = 20.0;
            float minHumidity = 60.0;

            if (iotHubClientHandle == NULL)
            {
                (void)printf("Failed on IoTHubClient_LL_Create\r\n");
            }
            else
            {
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
                // For mbed add the certificate information
                if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("failure to set option \"TrustedCerts\"\r\n");
                }
#endif // SET_TRUSTED_CERT_IN_SAMPLES


                ContosoAnemometer* myWeather = CREATE_MODEL_INSTANCE(WeatherStation, ContosoAnemometer);
                if (myWeather == NULL)
                {
                    (void)printf("Failed on CREATE_MODEL_INSTANCE\r\n");
                }
                else
                {
                    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, IoTHubMessage, myWeather) != IOTHUB_CLIENT_OK)
                    {
                        printf("unable to IoTHubClient_SetMessageCallback\r\n");
                    }
                    else
                    {
                        myWeather->DeviceId = "myFirstDevice";
                        myWeather->WindSpeed = avgWindSpeed + (rand() % 4 + 2);
                        myWeather->Temperature = minTemperature + (rand() % 10);
                        myWeather->Humidity = minHumidity + (rand() % 20);
                        {
                            unsigned char* destination;
                            size_t destinationSize;
                            if (SERIALIZE(&destination, &destinationSize, myWeather->DeviceId, myWeather->WindSpeed, myWeather->Temperature, myWeather->Humidity) != CODEFIRST_OK)
                            {
                                (void)printf("Failed to serialize\r\n");
                            }
                            else
                            {
                                sendMessage(iotHubClientHandle, destination, destinationSize, myWeather);
                                free(destination);
                            }
                        }

                        /* wait for commands */
                        while (1)
                        {
                            IoTHubClient_LL_DoWork(iotHubClientHandle);
                            ThreadAPI_Sleep(100);
                        }
                    }

                    DESTROY_MODEL_INSTANCE(myWeather);
                }
                IoTHubClient_LL_Destroy(iotHubClientHandle);
            }
            serializer_deinit();
        }
        platform_deinit();
    }
}
