// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include "badnetwork.h"
#include "iothubclient_common_e2e.h"
#include "iothubtransportamqp.h"
#include "testrunnerswitcher.h"
#include "network_disconnect.h"

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"

void disconnect_create_send_reconnect(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    D2C_MESSAGE_HANDLE d2cMessage;

    // Disconnect
    if (0 != network_disconnect())
    {
        ASSERT_FAIL("network disconnect failed");
    }

    // create
    iotHubClientHandle = client_connect_to_hub(deviceToUse, protocol);

    // Send the Event from the client
    d2cMessage = client_create_and_send_d2c(iotHubClientHandle);

    // reconnect
    if (0 != network_reconnect())
    {
        ASSERT_FAIL("network reconnect failed");
    }

    // Wait for confirmation that the event was recevied
    bool dataWasRecv = client_wait_for_d2c_confirmation(d2cMessage);
    ASSERT_IS_TRUE_WITH_MSG(dataWasRecv, "Failure sending data to IotHub"); // was received by the callback...

    // close the client connection
    IoTHubClient_Destroy(iotHubClientHandle);

    // Why?  Ask dan. 
    (void)platform_init();

    // Wait for the message to arrive
    service_wait_for_d2c_event_arrival(deviceToUse, d2cMessage);

    // cleanup
    destroy_d2c_message_handle(d2cMessage);
}

#define MESSAGE_COUNT 5
#define FIRST_MESSAGE_HEAD_START 15000
void disconnect_after_first_confirmation_then_close(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    D2C_MESSAGE_HANDLE d2cMessage[MESSAGE_COUNT];
    
    // create
    iotHubClientHandle = client_connect_to_hub(deviceToUse, protocol);

    // Send the first event from the client
    d2cMessage[0] = client_create_and_send_d2c(iotHubClientHandle);

    // give it a head start 
    ThreadAPI_Sleep(FIRST_MESSAGE_HEAD_START);

    // Send the rest
    for (int i=1; i<MESSAGE_COUNT;i++)
    {
        d2cMessage[i] = client_create_and_send_d2c(iotHubClientHandle);
    }

    // Wait for the first confirmation to arrive
    client_wait_for_d2c_confirmation(d2cMessage[0]);

    // disconnect and sleep for a while
    if (0 != network_disconnect())
    {
        ASSERT_FAIL("network disconnect failed");
    }

    // Verify that we haven't received all the confirmations.  Otherwise this test is invalid.  We want some pending confirmations
    printf("making sure we have at least one unconfirmed message before reconnecting\n");
    bool atLeastOneUnconfirmed = false;
    for (int i=0; i<MESSAGE_COUNT;i++)
    {
        bool confirmed = client_received_confirmation(d2cMessage[i]);
        printf ("message %d is %sconfirmed\n",i,confirmed ? "" : "not ");
        if (confirmed == false)
        {
            atLeastOneUnconfirmed = true;
            // break;
        }
    }
    ASSERT_IS_TRUE_WITH_MSG(atLeastOneUnconfirmed, "test needs one unconfirmed message to be valid.  Consider increasing FIRST_MESSAGE_HEAD_START.");
    
    // close the client
    IoTHubClient_Destroy(iotHubClientHandle);
    
    // cleanup
    for (int i=0; i<MESSAGE_COUNT;i++)
    {
        destroy_d2c_message_handle(d2cMessage[i]);
    }

    // finally reconnect 
    if (0 != network_reconnect())
    {
        ASSERT_FAIL("network reconnect failed");
    }
}

#define SLEEP_TIME_BETWEEN_STEPS 2000
#undef MESSAGE_COUNT
#define MESSAGE_COUNT 4
void send_disconnect_send_reconnect_etc(IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_HANDLE iotHubClientHandle;
    D2C_MESSAGE_HANDLE d2cMessage[MESSAGE_COUNT];
    
    // create
    iotHubClientHandle = client_connect_to_hub(deviceToUse, protocol);

    for (int i=0; i<MESSAGE_COUNT; i++)
    {
        d2cMessage[i] = client_create_and_send_d2c(iotHubClientHandle);
        ThreadAPI_Sleep(SLEEP_TIME_BETWEEN_STEPS);
        
        if (0 != network_disconnect())
        {
            ASSERT_FAIL("network disconnect failed");
        }
        ThreadAPI_Sleep(SLEEP_TIME_BETWEEN_STEPS);

        i++;
        d2cMessage[i] = client_create_and_send_d2c(iotHubClientHandle);
        ThreadAPI_Sleep(SLEEP_TIME_BETWEEN_STEPS);
        
        if (0 != network_reconnect())
        {
            ASSERT_FAIL("network reconnect failed");
        }
        ThreadAPI_Sleep(SLEEP_TIME_BETWEEN_STEPS);
    }
    
    // close the client connection
    IoTHubClient_Destroy(iotHubClientHandle);

    // Why?  Ask dan. 
    (void)platform_init();

    // Wait for the message to arrive
    for (int i=0; i<MESSAGE_COUNT; i++)
    {
        service_wait_for_d2c_event_arrival(deviceToUse, d2cMessage[i]);
        bool received = service_received_the_message(d2cMessage[i]);
        ASSERT_IS_TRUE_WITH_MSG(received, "Service did not receive all messages");
    }

    // cleanup
    for (int i=0; i<MESSAGE_COUNT;i++)
    {
        destroy_d2c_message_handle(d2cMessage[i]);
    }

}



