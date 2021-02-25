// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothub_pnp.h"
#include "parson.h"

IOTHUB_CLIENT_RESULT IoTHub_PnP_Serialize_ResponseProperties(
    const IOTHUB_PNP_RESPONSE_PROPERTY* responseProperties, 
    unsigned int numResponseProperties, 
    IOTHUB_PNP_DATA_SERIALIZED *responsePropertySerialized)
{
    (void)responseProperties;
    (void)numResponseProperties;
    (void)responsePropertySerialized;
    return 0;
}

IOTHUB_CLIENT_RESULT IoTHub_PnP_Serialize_ReportedProperties(
    const IOTHUB_PNP_REPORTED_PROPERTY* reportedProperties, 
    unsigned int numReportedProperties, 
    IOTHUB_PNP_DATA_SERIALIZED *reportedPropertySerialized)
{
    (void)reportedProperties;
    (void)numReportedProperties;
    (void)reportedPropertySerialized;
    return 0;
}

IOTHUB_CLIENT_RESULT IoTHub_PnP_Deserialize_UpdatedProperty(
                            const IOTHUB_PNP_DATA_SERIALIZED *updatedPropertySerialized, 
                            const char** pnpComponents, 
                            size_t numPnPComponents, 
                            IOTHUB_PNP_UPDATED_PROPERTY** pnpPropertyUpdated,
                            size_t* numPropertiesUpdated)
{
    (void)updatedPropertySerialized;
    (void)pnpComponents;
    (void)numPnPComponents;
    (void)pnpPropertyUpdated;
    (void)numPropertiesUpdated;
    return 0;
}

