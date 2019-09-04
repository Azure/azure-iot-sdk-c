// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothub_prov_transport.h"
#include "iothub_prov_mqtt_transport.h"
#include "azure_prov_client/prov_transport.h"

#include "iothubtransporthttp.h"
#include "azure_prov_client/prov_transport_http_client.h"

static IOTHUB_PROV_TRANSPORT_PROVIDER transport_func;

const IOTHUB_PROV_TRANSPORT_PROVIDER* IoTHub_Prov_MQTT_Protocol(void)
{
    transport_func.iothub_protocol = HTTP_Protocol();
    transport_func.prov_transport = Prov_Device_HTTP_Protocol();

    return &transport_func;
}

