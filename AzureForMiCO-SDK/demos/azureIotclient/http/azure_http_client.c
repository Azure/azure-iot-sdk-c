#include "mico.h"
#include "mico_iot_init.h"
#include "iothub_message.h"
#include "iothub_client.h"
#include "iothub_client_ll.h"
#include "iothubtransporthttp.h"
#include "platform_mico.h"

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
static const char* connectionString = "HostName=iothub-arthma.azure-devices.net;DeviceId=mico3165-arthma;SharedAccessKey=4veKB0VHHF9Hqa9MFOhbWaX0kFDcw/saEEaZLzsMx3o=";

static int callbackCounter;
static char msgText[1024];
static char propText[1024];
static bool g_continueRunning;
#define MESSAGE_COUNT       1
#define DOWORK_LOOP_NUM     1

typedef struct EVENT_INSTANCE_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    size_t messageTrackingId;  // For tracking the messages within the user callback.
} EVENT_INSTANCE;

static IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    int* counter = (int*)userContextCallback;
    const char* buffer;
    size_t size;
    MAP_HANDLE mapProperties;

    if (IoTHubMessage_GetByteArray(message, (const unsigned char**)&buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        mico_iot_azure_log("unable to retrieve the message data\r\n");
    }
    else
    {
        mico_iot_azure_log("Received Message [%d] with Data: <<<%.*s>>> & Size=%d\r\n", *counter, (int)size, buffer, (int)size);
        if (size == (strlen("quit") * sizeof(char)) && memcmp(buffer, "quit", size) == 0)
        {
            g_continueRunning = false;
        }
    }

    // Retrieve properties from the message
    mapProperties = IoTHubMessage_Properties(message);
    if (mapProperties != NULL)
    {
        const char*const* keys;
        const char*const* values;
        size_t propertyCount = 0;
        if (Map_GetInternals(mapProperties, &keys, &values, &propertyCount) == MAP_OK)
        {
            if (propertyCount > 0)
            {
                size_t index;

                mico_iot_azure_log("Message Properties:\r\n");
                for (index = 0; index < propertyCount; index++)
                {
                    mico_iot_azure_log("\tKey: %s Value: %s\r\n", keys[index], values[index]);
                }
                mico_iot_azure_log("\r\n");
            }
        }
    }

    /* Some device specific action code goes here... */
    (*counter)++;
    return IOTHUBMESSAGE_ACCEPTED;
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    EVENT_INSTANCE* eventInstance = (EVENT_INSTANCE*)userContextCallback;

    mico_iot_azure_log("Confirmation[%d] received for message tracking id = %u with result = %s\r\n",
                       callbackCounter,
                       eventInstance->messageTrackingId,
                       ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));

    /* Some device specific action code goes here... */
    callbackCounter++;
    IoTHubMessage_Destroy(eventInstance->messageHandle);
}

bool StartAzureIoTTest(void)
{
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

    EVENT_INSTANCE messages[MESSAGE_COUNT];
    double avgWindSpeed = 10.0;
    int receiveContext = 0;

    g_continueRunning = true;

    srand((unsigned int)time(NULL));

    callbackCounter = 0;

    if (platform_init() != 0)
    {
        mico_iot_azure_log("Failed to initialize the platform.\r\n");
    }
    else
    {
        mico_iot_azure_log("Starting the IoTHub client sample HTTP...\r\n");

        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol)) == NULL)
        {
            mico_iot_azure_log("ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
            unsigned int timeout = 241000;
            // Because it can poll "after 9 seconds" polls will happen effectively // at ~10 seconds.
            // Note that for scalabilty, the default value of minimumPollingTime
            // is 25 minutes. For more information, see:
            // https://azure.microsoft.com/documentation/articles/iot-hub-devguide/#messaging
            unsigned int minimumPollingTime = 9;
            if (IoTHubClient_LL_SetOption(iotHubClientHandle, "timeout", &timeout) != IOTHUB_CLIENT_OK)
            {
                mico_iot_azure_log("failure to set option \"timeout\"\r\n");
            }

            if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK)
            {
                mico_iot_azure_log("failure to set option \"MinimumPollingTime\"\r\n");
            }

#ifdef MBED_BUILD_TIMESTAMP
            // For mbed add the certificate information
            if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
            {
                mico_iot_azure_log("failure to set option \"TrustedCerts\"\r\n");
            }
#endif // MBED_BUILD_TIMESTAMP

            /* Setting Message call back, so we can receive Commands. */
            if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, &receiveContext) != IOTHUB_CLIENT_OK)
            {
                mico_iot_azure_log("ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");
            }
            else
            {
                mico_iot_azure_log("IoTHubClient_LL_SetMessageCallback...successful.\r\n");

                /* Now that we are ready to receive commands, let's send some messages */
                size_t iterator = 0;
                do
                {
                    if (iterator < MESSAGE_COUNT)
                    {
                        sprintf_s(msgText, sizeof(msgText), "{\"deviceId\": \"myFirstDevice\",\"windSpeed\": %.2f}", avgWindSpeed + (rand() % 4 + 2));
                        if ((messages[iterator].messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText))) == NULL)
                        {
                            mico_iot_azure_log("ERROR: iotHubMessageHandle is NULL!\r\n");
                        }
                        else
                        {
                            MAP_HANDLE propMap;

                            messages[iterator].messageTrackingId = iterator;

                            propMap = IoTHubMessage_Properties(messages[iterator].messageHandle);
                            (void)sprintf_s(propText, sizeof(propText), "PropMsg_%u", iterator);
                            if (Map_AddOrUpdate(propMap, "PropName", propText) != MAP_OK)
                            {
                                mico_iot_azure_log("ERROR: Map_AddOrUpdate Failed!\r\n");
                            }

                            if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messages[iterator].messageHandle, SendConfirmationCallback, &messages[iterator]) != IOTHUB_CLIENT_OK)
                            {
                                mico_iot_azure_log("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED!\r\n");
                            }
                            else
                            {
                                mico_iot_azure_log("IoTHubClient_LL_SendEventAsync accepted message [%u] for transmission to IoT Hub.\r\n", iterator);

                            }

                        }
                    }
                    IoTHubClient_LL_DoWork(iotHubClientHandle);
                    mico_thread_sleep(1);

                    iterator++;
                } while (g_continueRunning);

                mico_iot_azure_log("iothub_client_sample_http has gotten quit message, call DoWork %d more time to complete final sending...\r\n", DOWORK_LOOP_NUM);
                for (size_t index = 0; index < DOWORK_LOOP_NUM; index++)
                {
                    IoTHubClient_LL_DoWork(iotHubClientHandle);
                    mico_thread_sleep(1);
                }
            }
            IoTHubClient_LL_Destroy(iotHubClientHandle);
        }
        platform_deinit();
    }
    return true;
}

int application_start( void )
{
    OSStatus err = kNoErr;

    if(!mico_iot_init())
    {
        goto exit;
    }

    // Start Azure IoT MQTT testing
    mico_iot_azure_log( "Start Azure IoT testing..." );
    StartAzureIoTTest();

exit:
    mico_rtos_delete_thread(NULL);
    return err;
}
