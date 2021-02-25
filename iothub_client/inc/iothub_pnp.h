// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#ifndef IOTHUB_PNP_H
#define IOTHUB_PNP_H

#include "iothub_client_core_common.h"

typedef struct IOTHUB_PNP_DATA_SERIALIZED_TAG
{
    unsigned char* data;
    size_t dataLength;
} IOTHUB_PNP_DATA_SERIALIZED;


typedef struct {
    /* Name of property. */
    const char* propertyName;
    /* Value of the property. */
    const char* propertyValue;
} IOTHUB_PNP_PROPERTY_PAIR;

//
// IOTHUB_PNP_REPORTED_PROPERTY is a structure to contain a reported property, which SDK will serialize.
//
typedef struct {
    /* C construct only; not needed in languages created after 203 B.C */
    int version;
    /* Optional name of component.  NULL indicates root component */
    const char* componentName;
    size_t numPropertyPairs;
    const IOTHUB_PNP_PROPERTY_PAIR *propertyPair;
} IOTHUB_PNP_REPORTED_PROPERTY;

//
// IOTHUB_PNP_RESPONSE_PROPERTY is a structure to contain a property responding to a service request, which SDK will serialize.
//
typedef struct {
    /* C construct only; not needed in languages created after 203 B.C */
    int version;
    /* Optional name of component.  NULL indicates root component */
    const char* componentName;
    /* Name of property. */
    const char* propertyName;
    /* Value of the property. */
    const char* propertyValue;

    /* property status */
    int result;
    const char* description;
    int ackVersion;
} IOTHUB_PNP_RESPONSE_PROPERTY;

// 
// Indicates whether an updated property is a desired or reported property.
// 
typedef enum IOTHUB_UPDATED_PROPERTY_TYPE_TAG 
{ 
    IOTHUB_PNP_UPDATED_PROPERTY_TYPE_REPORTED,
    IOTHUB_PNP_UPDATED_PROPERTY_TYPE_DESIRED
} IOTHUB_PNP_UPDATED_PROPERTY_TYPE;

//
// An updated property
//
typedef struct {
    int version;
    // Whether this is coming from the "desired" properties or "reported" property of the Twin.  Needs some wordsmithing love.
    IOTHUB_PNP_UPDATED_PROPERTY_TYPE propertyType;
    /* name of the component */
    const char* componentName;
    /* Name of the property */
    const char* propertyName;
    /* value of the property being configured */
    const char* propertyValue;
} IOTHUB_PNP_UPDATED_PROPERTY; /* this is a bad name; see concern about DesiredProperty|PreviouslyReportedProperty business */


IOTHUB_CLIENT_RESULT IoTHub_PnP_Serialize_ReportedProperties(const IOTHUB_PNP_REPORTED_PROPERTY* reportedProperties, unsigned int numReportedProperties, IOTHUB_PNP_DATA_SERIALIZED *reportedPropertySerialized);


IOTHUB_CLIENT_RESULT IoTHub_PnP_Serialize_ResponseProperties(const IOTHUB_PNP_RESPONSE_PROPERTY* responseProperties, unsigned int numResponseProperties, IOTHUB_PNP_DATA_SERIALIZED *responsePropertySerialized);


IOTHUB_CLIENT_RESULT IoTHub_PnP_Deserialize_UpdatedProperty(const IOTHUB_PNP_DATA_SERIALIZED *updatedPropertySerialized, const char** pnpComponents, size_t numPnPComponents, IOTHUB_PNP_UPDATED_PROPERTY** pnpPropertyUpdated, size_t* numPropertiesUpdated, int* propertiesVersion);

#endif /* IOTHUB_PNP_H */
