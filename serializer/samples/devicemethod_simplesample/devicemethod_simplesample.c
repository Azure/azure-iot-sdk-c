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

//  How to use the sample with iothub-explorer (see: https://github.com/Azure/iothub-explorer):
//  Call method with Action:
//      iothub-explorer send device-name {\"Name\":\"TurnFanOff_with_Action\",\"Parameters\":{\"a\":3,\"b\":33}}
//  Call method with Method:
//      iothub-explorer device-method device-name TurnFanOff_with_Method "{\"a\":3, \"b\":33}"

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
static const char* connectionString = "HostName=...";

// Define the Model
BEGIN_NAMESPACE(WeatherStation);

DECLARE_MODEL(ContosoAnemometer,
WITH_DATA(ascii_char_ptr, DeviceId),
WITH_DATA(int, WindSpeed),
WITH_ACTION(TurnFanOn_with_Action),
WITH_ACTION(TurnFanOff_with_Action),
WITH_METHOD(TurnFanOn_with_Method),
WITH_METHOD(TurnFanOff_with_Method)
);

END_NAMESPACE(WeatherStation);

EXECUTE_COMMAND_RESULT TurnFanOn_with_Action(ContosoAnemometer* device)
{
    (void)device;
    (void)printf("Turning fan on with Action.\r\n");
    return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT TurnFanOff_with_Action(ContosoAnemometer* device)
{
    (void)device;
    (void)printf("Turning fan off with Action.\r\n");
    return EXECUTE_COMMAND_SUCCESS;
}

METHODRETURN_HANDLE TurnFanOn_with_Method(ContosoAnemometer* device)
{
    (void)device;
    (void)printf("Turning fan on with Method.\r\n");

    METHODRETURN_HANDLE result = MethodReturn_Create(1, "{\"Message\":\"Turning fan on with Method\"}");
    return result;
}

METHODRETURN_HANDLE TurnFanOff_with_Method(ContosoAnemometer* device)
{
    (void)device;
    (void)printf("Turning fan off with Method.\r\n");

    METHODRETURN_HANDLE result = MethodReturn_Create(0, "{\"Message\":\"Turning fan off with Method\"}");
    return result;
}

static int DeviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
{
    int result;

    /*this is step  3: receive the method and push that payload into serializer (from below)*/
    char* payloadZeroTerminated = (char*)malloc(size + 1);
    if (payloadZeroTerminated == 0)
    {
        printf("failed to malloc\r\n");
        *resp_size = 0;
        *response = NULL;
        result = -1;
    }
    else
    {
        (void)memcpy(payloadZeroTerminated, payload, size);
        payloadZeroTerminated[size] = '\0';

        /*execute method - userContextCallback is of type deviceModel*/
        METHODRETURN_HANDLE methodResult = EXECUTE_METHOD(userContextCallback, method_name, payloadZeroTerminated);
        free(payloadZeroTerminated);

        if (methodResult == NULL)
        {
            printf("failed to EXECUTE_METHOD\r\n");
            *resp_size = 0;
            *response = NULL;
            result = -1;
        }
        else
        {
            /* get the serializer answer and push it in the networking stack*/
            const METHODRETURN_DATA* data = MethodReturn_GetReturn(methodResult);
            if (data == NULL)
            {
                printf("failed to MethodReturn_GetReturn\r\n");
                *resp_size = 0;
                *response = NULL;
                result = -1;
            }
            else
            {
                result = data->statusCode;
                if (data->jsonValue == NULL)
                {
                    char* resp = "{}";
                    *resp_size = strlen(resp);
                    *response = (unsigned char*)malloc(*resp_size);
                    (void)memcpy(*response, resp, *resp_size);
                }
                else
                {
                    *resp_size = strlen(data->jsonValue);
                    *response = (unsigned char*)malloc(*resp_size);
                    (void)memcpy(*response, data->jsonValue, *resp_size);
                }
            }
            MethodReturn_Destroy(methodResult);
        }
    }
    return result;
}

void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    unsigned int messageTrackingId = (unsigned int)(uintptr_t)userContextCallback;

    (void)printf("Message Id: %u Received.\r\n", messageTrackingId);

    (void)printf("Result Call Back Called! Result is: %s \r\n", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size)
{
    static unsigned int messageTrackingId;
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
    if (messageHandle == NULL)
    {
        printf("unable to create a new IoTHubMessage\r\n");
    }
    else
    {
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
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    const unsigned char* buffer;
    size_t size;
    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        printf("unable to IoTHubMessage_GetByteArray\r\n");
        result = IOTHUBMESSAGE_ABANDONED;
    }
    else
    {
        /*buffer is not zero terminated*/
        char* temp = malloc(size + 1);
        if (temp == NULL)
        {
            printf("failed to malloc\r\n");
            result = IOTHUBMESSAGE_ABANDONED;
        }
        else
        {
            (void)memcpy(temp, buffer, size);
            temp[size] = '\0';
            EXECUTE_COMMAND_RESULT executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
            result =
                (executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
                (executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
                IOTHUBMESSAGE_REJECTED;
            free(temp);
        }
    }
    return result;
}

void devicemethod_simplesample_run(void)
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
                else if (IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, DeviceMethodCallback, myWeather) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("Failed on IoTHubClient_SetDeviceMethodCallback\r\n");
                }
                else
                {
                    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, IoTHubMessage, myWeather) != IOTHUB_CLIENT_OK)
                    {
                        printf("unable to IoTHubClient_SetMessageCallback\r\n");
                    }
                    else
                    {
                        myWeather->DeviceId = "myWeatherStation";
                        myWeather->WindSpeed = avgWindSpeed + (rand() % 4 + 2);
                        {
                            unsigned char* destination;
                            size_t destinationSize;
                            if (SERIALIZE(&destination, &destinationSize, myWeather->DeviceId, myWeather->WindSpeed) != CODEFIRST_OK)
                            {
                                (void)printf("Failed to serialize\r\n");
                            }
                            else
                            {
                                sendMessage(iotHubClientHandle, destination, destinationSize);
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
