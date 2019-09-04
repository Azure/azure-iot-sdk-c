// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothub_prov_transport.h"
#include "iothub_prov_mqtt_transport.h"
#include "azure_prov_client/prov_transport.h"

#include "iothubtransportmqtt.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"

static IOTHUB_PROV_TRANSPORT_PROVIDER transport_func =
{
    MQTT_Protocol,
    Prov_Device_MQTT_Protocol
};

const IOTHUB_PROV_TRANSPORT_PROVIDER* IoTHub_Prov_MQTT_Protocol(void)
{
    return &transport_func;
}

