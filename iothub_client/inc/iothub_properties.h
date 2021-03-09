// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#ifndef IOTHUB_PROPERTIES_H
#define IOTHUB_PROPERTIES_H

#include "iothub_client_core_common.h"

typedef struct {
    /* Name of property. */
    const char* name;
    /* Value of the property. */
    const char* value;
} IOTHUB_PROPERTY;

// 
// Indicates whether an updated property is a desired or reported property.
// 
typedef enum IOTHUB_WRITABLE_PROPERTY_TYPE_TAG 
{ 
    IOTHUB_WRITEABLE_PROPERTY_TYPE_REPORTED_FROM_DEVICE
    IOTHUB_WRITEABLE_PROPERTY_TYPE_WRITEABLE
} IOTHUB_WRITEABLE_PROPERTY_TYPE;

//
// A writeable property received from the service
//
typedef struct {
    /* Version of the structure */
    int version;
    /* Whether this properties */
    IOTHUB_WRITEABLE_PROPERTY_TYPE propertyType;
    /* name of the component */
    const char* componentName;
    /* Name of the property */
    const char* propertyName;
    /* value of the property being configured */
    const char* propertyValue;
} IOTHUB_WRITEABLE_PROPERTY;

//
// IOTHUB_WRITEABLE_PROPERTY_RESPONSE is a structure to contain a property responding to a service request.
//
typedef struct {
    /* Version of the structure */
    int version;
    /* Name of property. */
    const char* propertyName;
    /* Value of the property. */
    const char* propertyValue;
    /* property status */
    int result;
    /* Optional description */
    const char* description;
    /* Acknowledgement version */
    int ackVersion;
} IOTHUB_WRITEABLE_PROPERTY_RESPONSE;


IOTHUB_CLIENT_RESULT IoTHub_Serialize_Properties(const IOTHUB_PROPERTY* properties, size_t numProperties, const char* componentName, unsigned char** serializedProperties, size_t* serializedPropertiesLength);


IOTHUB_CLIENT_RESULT IoTHub_Serialize_ResponseToWriteableProperties(const IOTHUB_WRITEABLE_PROPERTY_RESPONSE* properties, size_t numProperties, const char* componentName, unsigned char** serializedProperties, size_t* serializedPropertiesLength);


// This is taking A LOT of parameters.  maybe time to move some into a common struct?
IOTHUB_CLIENT_RESULT IoTHub_Deserialize_WriteableProperty(IOTHUB_WRITEABLE_PROPERTY_PAYLOAD_TYPE payloadType, const unsigned char** serializedProperties, size_t* serializedPropertiesLength, const char** componentName, size_t numComponents, IOTHUB_WRITEABLE_PROPERTY** writeProperties, size_t* numWriteableProperties, int* propertiesVersion);

#endif /* IOTHUB_PROPERTIES_H */
