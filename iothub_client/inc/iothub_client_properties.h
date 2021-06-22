// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file    iothub_client_properties.h
*   @brief   APIs that serialize and deserialize properties modeled with DTDLv2.
*
*   @details Plug and Play devices are defined using the DTDLv2 modeling language described
*            at https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/dtdlv2.md.
*
*            Azure IoT Hub defines a set of conventions for serializing and deserializing DTDLv2 
*            properties that can be sent between Azure IoT Hub and devices connected to the Hub.
*
*            The format is JSON based.  It is possible to manually serialize and deserialize
*            this payload following the documented convention.  These APIs make this process easier.  
*            When serializing a property to be sent to IoT Hub, your application provides a C structure and 
*            the API returns a byte stream to be sent.
*
*            When receiving properties form IoT Hub, the deserialization API converts a raw stream
*            into an easier to process C structure with the JSON parsed out.
*
*            These APIs do not invoke any network I/O.  To actually send and receive data, these APIs 
*            will typically be paired with a corresponding IoT Hub device or module client.  
*  
*            Pseudocode to demonstrate the relationship for serializing properties and device clients:
*                   // Converts C structure into serialized stream.  
*                   IoTHubClient_Serialize_ReportedProperties(yourApplicationsPropertiesInStruct, &serializedByteStream);
*                   // Send the data
*                   IoTHubDeviceClient_LL_SendPropertiesAsync(deviceHandle, serializedByteStream);
*
*            Pseudocode to demonstrate the relationship for deserializing properties and device clients:
*                   // Request all device properties from IoT Hub
*                   IoTHubDeviceClient_LL_GetPropertiesAsync(deviceHandle, &yourAppCallback);
*                   // Time passes as request is processed...
*                   // ...
*                   // Your application's IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK is eventually invoked.  
*                   // Your application then will setup the iterator based on the data from the network.
*                   // rawDataFromIoTHub below corresponds to data your IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK 
*                   // receives.
*
*                   void yourAppCallback(rawDataFromIoTHub, ...) // your IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK implementation
*                   {
*                     IoTHubClient_Deserialize_Properties_CreateIterator(rawDataFromIoTHub, &iteratorHandle)
*
*                     // Enumerate each property that is in a component.  deserializedProperty will be of type
*                     // IOTHUB_CLIENT_DESERIALIZED_PROPERTY and is much easier to process than raw JSON 
*                     while (IoTHubClient_Deserialize_Properties_GetNextProperty(&iteratorHandle, &deserializedProperty)) {
*                         // Application processes deserializedProperty according to modeling rules
*                     }
*                   }
*/

#ifndef IOTHUB_CLIENT_PROPERTIES_H
#define IOTHUB_CLIENT_PROPERTIES_H

#include <stddef.h>

#include "iothub_client_core_common.h"

/** @brief Current version of @p IOTHUB_CLIENT_REPORTED_PROPERTY structure.  */
#define IOTHUB_CLIENT_REPORTED_PROPERTY_VERSION_1 1

/** @brief    This struct defines properties reported from the device.  This corresponds to what DTDLv2 calls a 
*             "read-only property."   This structure is filled out by the application and can be serialized into 
*             a payload for IoT Hub via @p IoTHubClient_Serialize_ReportedProperties(). */

typedef struct IOTHUB_CLIENT_REPORTED_PROPERTY_TAG {
    /** @brief   Version of the structure.  Currently must be IOTHUB_CLIENT_REPORTED_PROPERTY_VERSION_1. */
    int structVersion;
    /** @brief    Name of the property. */
    const char* name;
    /** @brief    Value of the property. */
    const char* value;
} IOTHUB_CLIENT_REPORTED_PROPERTY;

/** @brief Current version of @p IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE structure.  */
#define IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_VERSION_1 1

/** @brief    This struct represents the response to a writable property that the device will return.  
*             This structure is filled out by the application and can be serialized into a payload for IoT Hub via
*             @p IoTHubClient_Serialize_WritablePropertyResponse(). */
typedef struct IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_TAG {
    /** @brief   Version of the structure.  Currently must be IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE_VERSION_1. */
    int structVersion;
    /** @brief Name of the property. */
    const char* name;
    /** @brief Value of the property. */
    const char* value;
    /** @brief Result of the requested operation.  This maps to an HTTP status code.  */
    int result;
    /** @brief Acknowledgement version.  This corresponds to the version of the writable properties being responded to. */
    /** @details If using @p IoTHubClient_Deserialize_Properties_CreateIterator() to deserialize initial property request
    *            from IoT Hub, just set this field to what was returned in @p propertiesVersion. */
    int ackVersion;
    /** @brief Optional friendly description of the operation.  May be NULL. */
    const char* description;
} IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE;

/** @brief Enumeration that indicates whether a given property from the service was originally reported from the device
           (and hence the device application sees what the last reported value IoT Hub has)
           or whether this is a writable property that the service is requesting configuration for.
*/
typedef enum IOTHUB_PROPERTY_TYPE_TAG 
{
    /** @brief Property was initially reported from the device. */
    IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE,
    /** @brief Property is writable.  It is configured by service remotely. */
    IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE
} IOTHUB_CLIENT_PROPERTY_TYPE;

/* @brief Enumeration that indicates whether the JSON value of a deserialized property 
*         is a null-terminated string or binary.
*/
typedef enum IOTHUB_CLIENT_PROPERTY_VALUE_TYPE_TAG
{
    /* @brief Deserialized JSON value is a string. */
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING,
    /* @brief Deserialized JSON value is a binary. */
    IOTHUB_CLIENT_PROPERTY_VALUE_BINARY
} IOTHUB_CLIENT_PROPERTY_VALUE_TYPE;

/** @brief Current version of @p IOTHUB_CLIENT_DESERIALIZED_PROPERTY structure.  */
#define IOTHUB_CLIENT_DESERIALIZED_PROPERTY_VERSION_1 1

/** @brief    This struct represents a property from IoT Hub.  
              It is generated by IoTHubClient_Deserialize_Properties() while deserializing a property payload. */
typedef struct IOTHUB_CLIENT_DESERIALIZED_PROPERTY_TAG {
    /** @brief   Version of the structure.  Currently must be IOTHUB_CLIENT_DESERIALIZED_PROPERTY_VERSION_1. */
    int structVersion;
    /** @brief   Whether this is a property the device reported (and we're seeing what IoT Hub last received from 
         the device) or whether it's a writable property request from the device. */
    IOTHUB_CLIENT_PROPERTY_TYPE propertyType;
    /** @brief Name of the component.  Optional; may be NULL for the root component. */
    const char* componentName;
    /** @brief Name of the property. */
    const char* name;
    /** @brief Whether the value is passed as a string or non-null terminated length. */
    /** @description  Applications can safely assume that this will always be IOTHUB_CLIENT_PROPERTY_VALUE_STRING.  
    *                 They do not need to write code to handle IOTHUB_CLIENT_PROPERTY_VALUE_BINARY.
    *                 IOTHUB_CLIENT_PROPERTY_VALUE_BINARY may be supported by the SDK in the future, but 
    *                 IOTHUB_CLIENT_PROPERTY_VALUE_STRING will *always* by the default.
    */
    IOTHUB_CLIENT_PROPERTY_VALUE_TYPE valueType;
    /** @brief Value of the property. Currently this is only set to str. */
    union {
        const char* str;
        unsigned const char* bin;
    } value;
    /** @brief Number of bytes in valueLength.  Does not include null-terminator if IOTHUB_CLIENT_PROPERTY_VALUE_STRING 
    *          is set */
    size_t valueLength;
} IOTHUB_CLIENT_DESERIALIZED_PROPERTY;

/**
* @brief   Serializes reported properties into the required format for sending to IoT Hub.
*
* @param[in]   properties                  Pointer to IOTHUB_CLIENT_REPORTED_PROPERTY to be serialized.
* @param[in]   numProperties               Number of elements contained in @p properties.
* @param[in]   componentName               Optional component name these properties are part of.  May be NULL for default 
*                                          component.
* @param[out]  serializedProperties        Serialized output of @p properties for sending to IoT Hub.
*                                          The application must release this memory using 
*                                          IoTHubClient_Serialize_Properties_Destroy().
*                                          Note: This is NOT a null-terminated string.
* @param[out]  serializedPropertiesLength  Length of serializedProperties.
*
* @remarks  Applications can also manually construct the payload based on the convention rules.  This API is an easier 
*           to use alternative, so the application can use the more convenient IOTHUB_CLIENT_REPORTED_PROPERTY 
*           structure instead of raw serialization.
* 
*           This API does not perform any network I/O.  It only translates IOTHUB_CLIENT_REPORTED_PROPERTY into a byte 
*           array.  Applications should use APIs such as IoTHubDeviceClient_LL_SendPropertiesAsync to send the 
*           serialized data.
* 
* @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
IOTHUB_CLIENT_RESULT IoTHubClient_Serialize_ReportedProperties(
    const IOTHUB_CLIENT_REPORTED_PROPERTY* properties,
    size_t numProperties,
    const char* componentName,
    unsigned char** serializedProperties,
    size_t* serializedPropertiesLength);

/**
* @brief   Serializes the response to writable properties into the required format for sending to IoT Hub.  
*
* @param[in]   properties                 Pointer to IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE to be serialized.
* @param[in]   numProperties              Number of elements contained in @p properties.
* @param[in]   componentName              Optional component name these properties are part of.  May be NULL for 
*                                         default component.
* @param[out]  serializedProperties       Serialized output of @p properties for sending to IoT Hub.
                                          The application must release this memory using 
                                          IoTHubClient_Serialize_Properties_Destroy().
*                                         Note: This is NOT a null-terminated string.
* @param[out]  serializedPropertiesLength Length of serializedProperties
* 
* @remarks  Applications typically will invoke this API when processing a property from service 
*           (IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK) to indicate whether properties have been accepted or rejected 
*           by the device.  For example, if the service requested DesiredTemp=50, then the application could fill out 
*           the fields in an IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE structure indicating whether the request can be 
*           processed or whether there was a failure.
*
*           Applications can also manually construct the payload based on the convention rules.  This API is an easier 
*           to use alternative, so the application can use the more convenient IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE 
*           structure instead of raw serialization.
*
*           This API does not perform any network I/O.  It only translates IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE into
*           a byte array.  Applications should use APIs such as IoTHubDeviceClient_LL_SendPropertiesAsync to send the 
*           serialized data.
*
* @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
IOTHUB_CLIENT_RESULT IoTHubClient_Serialize_WritablePropertyResponse(
    const IOTHUB_CLIENT_WRITABLE_PROPERTY_RESPONSE* properties,
    size_t numProperties,
    const char* componentName,
    unsigned char** serializedProperties,
    size_t* serializedPropertiesLength);

/**
* @brief   Frees serialized properties that were initially allocated with IoTHubClient_Serialize_ReportedProperties() 
*          or IoTHubClient_Serialize_WritablePropertyResponse().
*
* @param[in]  serializedProperties Properties to free.
*/
void IoTHubClient_Serialize_Properties_Destroy(unsigned char* serializedProperties);

typedef struct IOTHUB_CLIENT_PROPERTY_ITERATOR_TAG* IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE;

/**
* @brief   Setup an iterator to enumerate through Plug and Play properties.
*
* @param[in]  payloadType              Whether the payload is a full set of properties or only a set of updated 
*                                      writable properties.
* @param[in]  payload                  Payload containing properties from Azure IoT that is to be deserialized. 
* @param[in]  payloadLength            Length of @p payload.
* @param[in]  componentsInModel        Optional array of components that correspond to the DTDLv2 model.  Can be NULL 
*                                      for models that don't contain sub-components.
* @param[in]  numComponentsInModel     Number of entries in @p componentsInModel.
* @param[out] propertyIteratorHandle   Returned handle used for subsequent iteration calls.
* @param[out] propertiesVersion        Returned version of the writable properties.  This is used when acknowledging a 
*                                      writable property update.
* 
* @remarks  Applications typically will invoke this API in their IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK 
*           implementation.  Many of the parameters this function takes should come directly from the 
*           IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK itself.
*
*           Applications must call @p IoTHubClient_Deserialize_Properties_DestroyIterator() to free @p 
*           propertyIteratorHandle on completion.
*
* @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
IOTHUB_CLIENT_RESULT IoTHubClient_Deserialize_Properties_CreateIterator(
    IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType,
    const unsigned char* payload,
    size_t payloadLength,
    const char** componentsInModel,
    size_t numComponentsInModel,
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE* propertyIteratorHandle,
    int* propertiesVersion);

/**
* @brief   Gets the next property during iteration.
*
* @param[in]   propertyIteratorHandle   Iteration handle returned by @p IoTHubClient_Deserialize_Properties_CreateIterator().
* @param[out]  property                 @p IOTHUB_CLIENT_DESERIALIZED_PROPERTY containing a deserialized representation 
*                                       of the properties.
* @param[out]  propertySpecified        Returned value indicating whether a property was found or not.  If false, this 
*                                       indicates all components have been iterated over.
*
* @remarks  Applications must call @p IoTHubClient_Deserialize_Properties_DestroyProperty() to free the @p property returned 
*           by this call.
*
* @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
IOTHUB_CLIENT_RESULT IoTHubClient_Deserialize_Properties_GetNextProperty(
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE propertyIteratorHandle,
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property,
    bool* propertySpecified);

/**
* @brief   Frees memory allocated by IoTHubClient_Deserialize_Properties().
*
* @param   property  IOTHUB_CLIENT_DESERIALIZED_PROPERTY initially allocated by IoTHubClient_Deserialize_Properties() 
*                    to be freed.
* 
*/
void IoTHubClient_Deserialize_Properties_DestroyProperty(
    IOTHUB_CLIENT_DESERIALIZED_PROPERTY* property);

/**
* @brief   Frees memory allocated by IoTHubClient_Deserialize_Properties_CreateIterator().
*
* @param   propertyIteratorHandle   IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE initially allocated by @p 
*                                   IoTHubClient_Deserialize_Properties_CreateIterator() to be freed.
* 
*/
void IoTHubClient_Deserialize_Properties_DestroyIterator(
    IOTHUB_CLIENT_PROPERTY_ITERATOR_HANDLE propertyIteratorHandle);

#endif /* IOTHUB_CLIENT_PROPERTIES_H */
