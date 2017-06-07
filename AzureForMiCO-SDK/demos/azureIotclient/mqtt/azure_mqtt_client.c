#include "mico.h"
#include "iothub_message.h"
#include "iothub_client.h"
#include "iothub_client_ll.h"
#include "iothubtransportmqtt.h"
#include "platform_mico.h"
#include "mico_time.h"
#include "certs.h"

#define azure_client_mqtt_log(M, ...) custom_log("MQTT", M, ##__VA_ARGS__)

//
// Please get your IoT hub connection string from Azure portal, the string should be like this:
//   HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>
//
static const char* connectionString = "[Azure IoT Hub connection string]";

static int callbackCounter;

#define MESSAGE_COUNT   5
#define DOWORK_LOOP_NUM 10

static mico_semaphore_t wait_sem = NULL;

//
// Register your develop board's mac address in https://openwifi/?Area=APAC,
// waiting for 5 mins and then you can get the internet access
//
const char* ap_ssid = "MSFTLAB";
const char* ap_wep_pwd = "";

typedef struct EVENT_INSTANCE_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    size_t messageTrackingId;  // For tracking the messages within the user callback.
} EVENT_INSTANCE;

static void micoNotify_ConnectFailedHandler(OSStatus err, void* inContext)
{
    azure_client_mqtt_log("Failed to join '%s': %d", ap_ssid, err);
}

static void micoNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    UNUSED_PARAMETER( inContext );
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            azure_client_mqtt_log("%s was connected!", ap_ssid);
            mico_rtos_set_semaphore( &wait_sem );
            break;
        case NOTIFY_STATION_DOWN:
            azure_client_mqtt_log("%s was disconnected!", ap_ssid);
            break;
        default:
            break;
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    int* counter = (int*)userContextCallback;
    const char* buffer;
    size_t size;

    if (IoTHubMessage_GetByteArray(message, (const unsigned char**)&buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        azure_client_mqtt_log("unable to retrieve the message data\r\n");
    }
    else
    {
        azure_client_mqtt_log("Received Message [%d] with Data: <<<%.*s>>> & Size=%d\r\n", *counter, (int)size, buffer, (int)size);
    }

    // Retrieve properties from the message
    MAP_HANDLE mapProperties = IoTHubMessage_Properties(message);
    if (mapProperties != NULL)
    {
        const char*const* keys;
        const char*const* values;
        size_t propertyCount = 0;
        if (Map_GetInternals(mapProperties, &keys, &values, &propertyCount) == MAP_OK)
        {
            if (propertyCount > 0)
            {
                for (size_t index = 0; index < propertyCount; index++)
                {
                    azure_client_mqtt_log("\tKey: %s Value: %s\r\n", keys[index], values[index]);
                }
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
    azure_client_mqtt_log("Confirmation[%d] received for message tracking id = %u with result = %d\r\n", callbackCounter, eventInstance->messageTrackingId, result);
    /* Some device specific action code goes here... */
    callbackCounter++;
    IoTHubMessage_Destroy(eventInstance->messageHandle);
}

static void StartAzureIoTTest()
{
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

    char msgText[128];
    char propText[32];
    EVENT_INSTANCE messages[MESSAGE_COUNT];
    micoMemInfo_t* pInfo;

    srand((unsigned int)time(NULL));
    double avgWindSpeed = 10.0;

    callbackCounter = 0;
    int receiveContext = 0;

    // Try to connect the Azure IoT hub
    for (int i = 0; i < 10; i++)
    {
        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol)) != NULL)
        {
            break;
        }
    }
    if(iotHubClientHandle == NULL)
    {
        azure_client_mqtt_log("ERROR: iotHubClientHandle is NULL!\r\n");
        return;
    }

    bool traceOn = true;
    IoTHubClient_LL_SetOption(iotHubClientHandle, "logtrace", &traceOn);

    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
    {
        azure_client_mqtt_log("failure to set option \"TrustedCerts\"\r\n");
    }


    /* Setting Message call back, so we can receive Commands. */
    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, &receiveContext) != IOTHUB_CLIENT_OK)
    {
       azure_client_mqtt_log("ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");
    }
    else
    {
       azure_client_mqtt_log("IoTHubClient_LL_SetMessageCallback...successful.\r\n");

       /* Now that we are ready to receive commands, let's send some messages */
       for (int iterator = 0; iterator < MESSAGE_COUNT; iterator++)
       {
           sprintf_s(msgText, sizeof(msgText), "{\"deviceId\":\"mico dev kit\",\"windSpeed\":%.2f}", avgWindSpeed + (rand() % 4 + 2));
           if ((messages[iterator].messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText))) == NULL)
           {
               azure_client_mqtt_log("ERROR: iotHubMessageHandle is NULL!\r\n");
           }
           else
           {
               messages[iterator].messageTrackingId = iterator;
               MAP_HANDLE propMap = IoTHubMessage_Properties(messages[iterator].messageHandle);
               (void)sprintf_s(propText, sizeof(propText), "PropMsg_%u", iterator);
               if (Map_AddOrUpdate(propMap, "PropName", propText) != MAP_OK)
               {
                   azure_client_mqtt_log("ERROR: Map_AddOrUpdate Failed!\r\n");
               }

               if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messages[iterator].messageHandle, SendConfirmationCallback, &messages[iterator]) != IOTHUB_CLIENT_OK)
               {
                   azure_client_mqtt_log("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED!\r\n");
               }
               else
               {
                   azure_client_mqtt_log("IoTHubClient_LL_SendEventAsync accepted message [%d] for transmission to IoT Hub.\r\n", (int)iterator);
               }
           }

           IoTHubClient_LL_DoWork(iotHubClientHandle);
           pInfo =  MicoGetMemoryInfo();
           azure_client_mqtt_log("****Memory info: total %d, free %d.", pInfo->total_memory, pInfo->free_memory);
           mico_thread_sleep(1);
       }

       azure_client_mqtt_log("iothub_client_sample_mqtt has gotten quit message, call DoWork %d more time to complete final sending...\r\n", DOWORK_LOOP_NUM);
       for (size_t index = 0; index < DOWORK_LOOP_NUM; index++)
       {
           IoTHubClient_LL_DoWork(iotHubClientHandle);
           mico_thread_sleep(1);
       }

       // Print out the memory usage
       pInfo =  MicoGetMemoryInfo();
       azure_client_mqtt_log("****Memory info: total %d, free %d.", pInfo->total_memory, pInfo->free_memory);
    }

    IoTHubClient_LL_Destroy(iotHubClientHandle);
    azure_client_mqtt_log("Azure IoT MQTT testing has done.");
}

void mqtt_client_thread( mico_thread_arg_t arg )
{
    UNUSED_PARAMETER( arg );

    StartAzureIoTTest();

    mico_rtos_delete_thread( NULL );
}

int application_start( void )
{
    OSStatus err = kNoErr;
    network_InitTypeDef_adv_st  wNetConfigAdv;
    micoMemInfo_t* pInfo;

    init_mico_time();

    // Initialization
    MicoInit( );

    pInfo =  MicoGetMemoryInfo();
    azure_client_mqtt_log("****Memory info: total %d, free %d.", pInfo->total_memory, pInfo->free_memory);

    // Start auto sync with NTP server
    start_sntp();

    mico_rtos_init_semaphore( &wait_sem, 1 );

    /* Register user function when wlan connection status is changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler, NULL );
    require_noerr( err, exit );

    /* Register user function when wlan connection is faild in one attempt */
    err = mico_system_notify_register( mico_notify_WIFI_CONNECT_FAILED, (void *)micoNotify_ConnectFailedHandler, NULL );
    require_noerr( err, exit );

    IPStatusTypedef para;
    micoWlanGetIPStatus(&para, 1);

    azure_client_mqtt_log("mac address: %s.", (char*)para.mac);

    /* Initialize wlan parameters */
    memset( &wNetConfigAdv, 0x0, sizeof(wNetConfigAdv) );
    strcpy((char*)wNetConfigAdv.ap_info.ssid, ap_ssid);           /* wlan ssid string */
    strcpy((char*)wNetConfigAdv.key, ap_wep_pwd);                 /* wlan key string or hex data in WEP mode */
    wNetConfigAdv.key_len = strlen(ap_wep_pwd);                   /* wlan key length */
    wNetConfigAdv.ap_info.security = SECURITY_TYPE_AUTO;          /* wlan security mode */
    wNetConfigAdv.ap_info.channel = 0;                            /* Select channel automatically */
    wNetConfigAdv.dhcpMode = DHCP_Client;                         /* Fetch Ip address from DHCP server */
    wNetConfigAdv.wifi_retry_interval = 100;                      /* Retry interval after a failure connection */

    /* Connect Now! */
    azure_client_mqtt_log("connecting to %s...", wNetConfigAdv.ap_info.ssid);
    micoWlanStartAdv(&wNetConfigAdv);

    /* Wait for wlan connection*/
    mico_rtos_get_semaphore( &wait_sem, MICO_WAIT_FOREVER );

    pInfo =  MicoGetMemoryInfo();
    azure_client_mqtt_log("****Memory info: total %d, free %d.", pInfo->total_memory, pInfo->free_memory);

    // Start Azure IoT MQTT testing
    azure_client_mqtt_log( "Start Azure IoT MQTT testing..." );
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "mqtt_client", mqtt_client_thread, 0x4000, 0 );
    require_noerr_string( err, exit, "ERROR: Unable to start the mqtt client thread." );

exit:
    if (wait_sem != NULL)
    {
        mico_rtos_deinit_semaphore(&wait_sem);
        wait_sem = NULL;
    }
    mico_rtos_delete_thread(NULL);
    return err;
}
