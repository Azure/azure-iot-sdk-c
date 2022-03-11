// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file    iothub_client_properties.h
*   @brief   APIs that create and parse properties modeled with DTDLv2.
*
*   @details Plug and Play devices are defined using the DTDLv2 modeling language described
*            at https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/dtdlv2.md.
*
*            Azure IoT Hub defines a set of conventions for parsing and creating DTDLv2 
*            properties that can be sent between Azure IoT Hub and devices connected to the Hub.
*
*            The format is JSON based.  It is possible to manually serialize and deserialize
*            this payload following the documented convention.  These APIs make this process easier.  
*            When creating a property to be sent to IoT Hub, your application provides a C structure and 
*            the API returns a byte stream to be sent.
*
*            When receiving properties form IoT Hub, the parsing API converts a raw stream
*            into an easier to process C structure with the JSON parsed out.
*
*            These APIs do not invoke any network I/O.  To actually send and receive data, these APIs 
*            will typically be paired with a corresponding IoT Hub device or module client.  
*  
*            Pseudocode to demonstrate the relationship for creating properties and device clients:
*                   // Converts C structure into serialized stream.  
*                   IoTHubClient_Properties_Writer_CreateReported(&yourApplicationsPropertiesInStruct, &serializedByteStream);
*                   // Send the data
*                   IoTHubDeviceClient_LL_SendPropertiesAsync(deviceHandle, serializedByteStream);
*
*            Pseudocode to demonstrate the relationship for deserializing properties and device clients:
*                   // Request all device properties from IoT Hub
*                   IoTHubDeviceClient_LL_GetPropertiesAsync(deviceHandle, &yourAppCallback);
*                   // Time passes as request is processed...
*                   // ...
*                   // Your application's IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK is eventually invoked.  
*                   // Your application then will setup the parser handle based on the data from the network.
*                   // rawDataFromIoTHub below corresponds to data your IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK 
*                   // receives.
*
*                   void yourAppCallback(rawDataFromIoTHub, ...) // your IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK implementation
*                   {
*                     IoTHubClient_Properties_Parser_Create(rawDataFromIoTHub, &parserHandle)
*
*                     // Enumerate each property that is in a component.  deserializedProperty will be of type
*                     // IOTHUB_CLIENT_PROPERTY_PARSED and is much easier to process than raw JSON 
*                     while (IoTHubClient_Properties_Parser_GetNext(&parserHandle, &deserializedProperty)) {
*                         // Application processes deserializedProperty according to modeling rules
*                         // ...
*                         // Frees memory allocated by IoTHubClient_Properties_Parser_GetNext
*                         IoTHubClient_Properties_ParsedProperty_Destroy(&deserializedProperty);
*                     }
*                     // Frees memory allocated by IoTHubClient_Properties_Parser_Create
*                     IoTHubClient_Properties_Parser_Destroy(parserHandle);
*                   }
*/

#ifndef IOTHUB_CLIENT_PROPERTIES_H
#define IOTHUB_CLIENT_PROPERTIES_H

#include "umock_c/umock_c_prod.h"

#include <stddef.h>

#include "iothub_client_core_common.h"

/** @brief Current version of @p IOTHUB_CLIENT_PROPERTY_REPORTED structure.  */
#define IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1 1

/** @brief    This struct defines properties reported from the device.  This corresponds to what DTDLv2 calls a 
*             "read-only property."   This structure is filled out by the application and can be serialized into 
*             a payload for IoT Hub via @p IoTHubClient_Properties_Writer_CreateReported(). */

typedef struct IOTHUB_CLIENT_PROPERTY_REPORTED_TAG {
    /** @brief   Version of the structure.  Currently must be IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1. */
    int structVersion;
    /** @brief    Name of the property. */
    const char* name;
    /** @brief    Value of the property.  This must be legal JSON.  Strings need to be escaped by the application, e.g. reported.value="\"MyValue\""; */
    const char* value;
} IOTHUB_CLIENT_PROPERTY_REPORTED;

/** @brief Current version of @p IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE structure.  */
#define IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_STRUCT_VERSION_1 1

/** @brief    This struct represents the response to a writable property that the device will return.  
*             This structure is filled out by the application and can be serialized into a payload for IoT Hub via
*             @p IoTHubClient_Properties_Write_WritableResponse(). */
typedef struct IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_TAG {
    /** @brief   Version of the structure.  Currently must be IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_STRUCT_VERSION_1. */
    int structVersion;
    /** @brief Name of the property. */
    const char* name;
    /** @brief Value of the property.   This must be legal JSON.  Strings need to be escaped by the application, e.g. writeableResponse.value="\"MyValue\""; */
    const char* value;
    /** @brief Result of the requested operation.  This maps to an HTTP status code.  */
    int result;
    /** @brief Acknowledgement version.  This corresponds to the version of the writable properties being responded to. */
    /** @details If using @p IoTHubClient_Properties_Parser_CreateIterator() to deserialize initial property request
    *            from IoT Hub, just set this field to what was returned in @p propertiesVersion. */
    int ackVersion;
    /** @brief Optional friendly description of the operation.  May be NULL. */
    const char* description;
} IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE;

/** @brief Enumeration that indicates whether a given property from the service was originally reported from the device
           (and hence the device application sees what the last reported value IoT Hub has)
           or whether this is a writable property that the service is requesting configuration for.
*/
#define IOTHUB_CLIENT_PROPERTY_TYPE_VALUES \
    IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_DEVICE, \
    IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE

MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_PROPERTY_TYPE, IOTHUB_CLIENT_PROPERTY_TYPE_VALUES);

/* @brief Enumeration that indicates whether the JSON value of a deserialized property 
*         is a null-terminated string or binary.  Currently only STRING values are supported.
*/
#define IOTHUB_CLIENT_PROPERTY_VALUE_TYPE_VALUES \
    IOTHUB_CLIENT_PROPERTY_VALUE_STRING

MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_PROPERTY_VALUE_TYPE, IOTHUB_CLIENT_PROPERTY_VALUE_TYPE_VALUES);

/** @brief Current version of @p IOTHUB_CLIENT_PROPERTY_PARSED structure.  */
#define IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1 1

/** @brief    This struct represents a property from IoT Hub.  
              It is generated by IoTHubClient_Deserialize_Properties() while deserializing a property payload. */
typedef struct IOTHUB_CLIENT_PROPERTY_PARSED_TAG {
    /** @brief   Version of the structure.  Currently must be IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1. */
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
    *                 Additional types may be added in the future but they will never be the default.
    */
    IOTHUB_CLIENT_PROPERTY_VALUE_TYPE valueType;
    /** @brief Value of the property. Currently this is only set to str. */
    union {
        const char* str;
    } value;
    /** @brief Number of bytes in valueLength.  Does not include null-terminator if IOTHUB_CLIENT_PROPERTY_VALUE_STRING 
    *          is set */
    size_t valueLength;
} IOTHUB_CLIENT_PROPERTY_PARSED;

/**
* @brief   Writes the reported properties into bytes for sending to IoT Hub.
*
* @param[in]   properties                  Pointer to IOTHUB_CLIENT_PROPERTY_REPORTED to be serialized.
* @param[in]   numProperties               Number of elements contained in @p properties.
* @param[in]   componentName               Optional component name these properties are part of.  May be NULL for default 
*                                          component.
* @param[out]  serializedProperties        Serialized output of @p properties for sending to IoT Hub.
*                                          The application must release this memory using 
*                                          IoTHubClient_Properties_Properties_Writer_Destroy().
*                                          Note: This is NOT a null-terminated string.
* @param[out]  serializedPropertiesLength  Length of serializedProperties.
*
* @remarks  Applications can also manually construct the payload based on the convention rules.  This API is an easier 
*           to use alternative, so the application can use the more convenient IOTHUB_CLIENT_PROPERTY_REPORTED 
*           structure instead of raw serialization.
* 
*           This API does not perform any network I/O.  It only translates IOTHUB_CLIENT_PROPERTY_REPORTED into a byte 
*           array.  Applications should use APIs such as IoTHubDeviceClient_LL_SendPropertiesAsync to send the 
*           serialized data.
* 
* @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_Properties_Writer_CreateReported, const IOTHUB_CLIENT_PROPERTY_REPORTED*, properties, size_t, numProperties, const char*, componentName, unsigned char**, serializedProperties, size_t*, serializedPropertiesLength);

/**
* @brief   Writes the response to writable properties into bytes for sending to IoT Hub.
*
* @param[in]   properties                 Pointer to IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE to be serialized.
* @param[in]   numProperties              Number of elements contained in @p properties.
* @param[in]   componentName              Optional component name these properties are part of.  May be NULL for 
*                                         default component.
* @param[out]  serializedProperties       Serialized output of @p properties for sending to IoT Hub.
                                          The application must release this memory using 
                                          IoTHubClient_Properties_Properties_Writer_Destroy().
*                                         Note: This is NOT a null-terminated string.
* @param[out]  serializedPropertiesLength Length of serializedProperties
* 
* @remarks  Applications typically will invoke this API when processing a property from service 
*           (IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK) to indicate whether properties have been accepted or rejected 
*           by the device.  For example, if the service requested DesiredTemp=50, then the application could fill out 
*           the fields in an IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE structure indicating whether the request can be 
*           processed or whether there was a failure.
*
*           Applications can also manually construct the payload based on the convention rules.  This API is an easier 
*           to use alternative, so the application can use the more convenient IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE 
*           structure instead of raw serialization.
*
*           This API does not perform any network I/O.  It only translates IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE into
*           a byte array.  Applications should use APIs such as IoTHubDeviceClient_LL_SendPropertiesAsync to send the 
*           serialized data.
*
* @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_Properties_Writer_CreateWritableResponse, const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE*, properties, size_t, numProperties, const char*, componentName, unsigned char**, serializedProperties, size_t*, serializedPropertiesLength);

/**
* @brief   Frees serialized properties that were initially allocated with IoTHubClient_Properties_Writer_CreateReported() 
*          or IoTHubClient_Properties_Write_WritableResponse().
*
* @param[in]  serializedProperties Properties to free.
*/
MOCKABLE_FUNCTION(, void, IoTHubClient_Properties_Properties_Writer_Destroy, unsigned char*, serializedProperties);

typedef struct IOTHUB_CLIENT_PROPERTY_ITERATOR_TAG* IOTHUB_CLIENT_PROPERTIES_PARSER_HANDLE;

/**
* @brief   Setup an iterator to parse through Plug and Play properties recevied from IoT Hub.
*
* @param[in]  payloadType              Whether the payload is a full set of properties or only a set of updated 
*                                      writable properties.
* @param[in]  payload                  Payload containing properties from Azure IoT that is to be deserialized. 
* @param[in]  payloadLength            Length of @p payload.
* @param[out] propertyIteratorHandle   Returned handle used for subsequent iteration calls.
* 
* @remarks  Applications typically will invoke this API in their IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK 
*           implementation.  Many of the parameters this function takes should come directly from the 
*           IOTHUB_CLIENT_PROPERTIES_RECEIVED_CALLBACK itself.
*
*           Applications must call @p IoTHubClient_Properties_Parser_Destroy() to free @p 
*           propertyIteratorHandle on completion.
*
* @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_Properties_Parser_Create,  IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE, payloadType, const unsigned char*, payload, size_t, payloadLength, IOTHUB_CLIENT_PROPERTIES_PARSER_HANDLE*, propertyIteratorHandle);

/**
* @brief   Retrieves the version associated with the properties payload.
*
* @param[in]   propertyIteratorHandle   Iteration handle returned by @p IoTHubClient_Properties_Parser_Create().
* @param[out]  propertiesVersion        Version of the writable properties received.  This is used when acknowledging a 
*                                       writable property update.
*
* @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_Properties_Parser_GetVersion, IOTHUB_CLIENT_PROPERTIES_PARSER_HANDLE, propertyIteratorHandle, int*, propertiesVersion);

/**
* @brief   Gets the next property during iteration.
*
* @param[in]   propertyIteratorHandle   Iteration handle returned by @p IoTHubClient_Properties_Parser_Create().
* @param[out]  property                 @p IOTHUB_CLIENT_PROPERTY_PARSED containing a deserialized representation 
*                                       of the properties.
* @param[out]  propertySpecified        Returned value indicating whether a property was found or not.  If false, this 
*                                       indicates all components have been iterated over.
*
* @remarks  Applications must call @p IoTHubClient_Properties_ParsedProperty_Destroy() to free the @p property returned by this call.
*           The property structure becomes invalid after @p IoTHubClient_Properties_ParsedProperty_Destroy has been called.
*
* @return   IOTHUB_CLIENT_OK upon success or an error code upon failure.
*/
MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_Properties_Parser_GetNext, IOTHUB_CLIENT_PROPERTIES_PARSER_HANDLE, propertyIteratorHandle, IOTHUB_CLIENT_PROPERTY_PARSED*, property, bool*, propertySpecified);

/**
* @brief   Frees memory allocated by IoTHubClient_Properties_Parser_GetNext().
*
* @param   property  IOTHUB_CLIENT_PROPERTY_PARSED initially allocated by IoTHubClient_Deserialize_Properties() 
*                    to be freed.
* 
*/
MOCKABLE_FUNCTION(, void, IoTHubClient_Properties_ParsedProperty_Destroy,  IOTHUB_CLIENT_PROPERTY_PARSED*, property);

/**
* @brief   Frees memory allocated by IoTHubClient_Properties_Parser_Create().
*
* @param   propertyIteratorHandle   IOTHUB_CLIENT_PROPERTIES_PARSER_HANDLE initially allocated by @p 
*                                   IoTHubClient_Properties_Parser_Create() to be freed.
* @remarks After an application calls IoTHubClient_Properties_Parser_Destroy, any previous 
*          IOTHUB_CLIENT_PROPERTY_PARSED structures that were retrieved via
*          IoTHubClient_Properties_Parser_GetNext become invalid.
* 
*/
MOCKABLE_FUNCTION(, void, IoTHubClient_Properties_Parser_Destroy, IOTHUB_CLIENT_PROPERTIES_PARSER_HANDLE, propertyIteratorHandle);

#endif /* IOTHUB_CLIENT_PROPERTIES_H */
