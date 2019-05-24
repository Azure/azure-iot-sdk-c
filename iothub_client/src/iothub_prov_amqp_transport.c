// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothub_prov_transport.h"
#include "iothub_prov_amqp_transport.h"
#include "azure_prov_client/prov_transport.h"

#include "iothubtransportamqp.h"
#include "azure_prov_client/prov_transport_amqp_client.h"

static IOTHUB_PROV_TRANSPORT_PROVIDER transport_func =
{
    AMQP_Protocol,
    Prov_Device_AMQP_Protocol
};

const IOTHUB_PROV_TRANSPORT_PROVIDER* IoTHub_Prov_AMQP_Protocol(void)
{
    return &transport_func;
}
