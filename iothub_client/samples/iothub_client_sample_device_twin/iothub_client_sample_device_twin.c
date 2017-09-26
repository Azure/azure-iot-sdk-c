// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "iothub_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "iothubtransportmqtt.h"
#include "iothubtransportamqp.h"

#ifdef MBED_BUILD_TIMESTAMP
#include "certs.h"
#endif // MBED_BUILD_TIMESTAMP

DEFINE_ENUM_STRINGS(DEVICE_TWIN_UPDATE_STATE, DEVICE_TWIN_UPDATE_STATE_VALUES);


/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
static const char* connectionString = "HostName=iot-sdks-test.azure-devices.net;DeviceId=ewertons-device2;SharedAccessKey=yfIId8422KUMOnDuvhDiz54rg2fQ1Hndbwd/XPh5PUo=";

static char msgText[1024];
static char propText[1024];
static bool g_continueRunning;
#define DOWORK_LOOP_NUM     3

static void deviceTwinCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    (void)userContextCallback;

    printf("Device Twin update received (state=%s, size=%lu): %s\r\n", 
        ENUM_TO_STRING(DEVICE_TWIN_UPDATE_STATE, update_state), size, payLoad);
}

static void reportedStateCallback(int status_code, void* userContextCallback)
{
    (void)userContextCallback;
    printf("Device Twin reported properties update completed with result: %d\r\n", status_code);

    g_continueRunning = false;
}


void iothub_client_sample_device_twin_run(void)
{
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

    g_continueRunning = true;
    
    if (platform_init() != 0)
    {
        (void)printf("Failed to initialize the platform.\r\n");
    }
    else
    {
        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
            bool traceOn = true;
            const char* reportedState = "{ 'device_property': 'new_value'}";
            size_t reportedStateSize = strlen(reportedState);

            (void)IoTHubClient_LL_SetOption(iotHubClientHandle, "logtrace", &traceOn);

#ifdef MBED_BUILD_TIMESTAMP
            // For mbed add the certificate information
            if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
            {
                printf("failure to set option \"TrustedCerts\"\r\n");
            }
#endif // MBED_BUILD_TIMESTAMP

            // Check the return of all API calls when developing your solution. Return checks ommited for sample simplification.

            (void)IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, deviceTwinCallback, iotHubClientHandle);
            (void)IoTHubClient_LL_SendReportedState(iotHubClientHandle, (const unsigned char*)reportedState, reportedStateSize, reportedStateCallback, iotHubClientHandle);

            do
            {
                IoTHubClient_LL_DoWork(iotHubClientHandle);
                ThreadAPI_Sleep(1);
            } while (g_continueRunning);

            for (size_t index = 0; index < DOWORK_LOOP_NUM; index++)
            {
                IoTHubClient_LL_DoWork(iotHubClientHandle);
                ThreadAPI_Sleep(1);
            }

            IoTHubClient_LL_Destroy(iotHubClientHandle);
        }
        platform_deinit();
    }
}

int main(void)
{
    iothub_client_sample_device_twin_run();
    return 0;
}
