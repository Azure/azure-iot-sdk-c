# uamqp_messaging Requirements


## Overview

This module is a helper to convert message structures supported by IoTHub client (IOTHUB_MESSAGE_HANDLE) to uAMQP (MESSAGE_HANDLE) and vice-versa. 

## Exposed API

```c
extern int message_create_IoTHubMessage_from_uamqp_message(MESSAGE_HANDLE uamqp_message, IOTHUB_MESSAGE_HANDLE* iothubclient_message);
extern int message_create_uamqp_encoding_from_iothub_message(IOTHUB_MESSAGE_HANDLE message_handle, BINARY_DATA* body_binary_data);
```


### message_create_IoTHubMessage_from_uamqp_message

Creates an IOTHUB_MESSAGE_HANDLE instance which represents the same message defined by the MESSAGE_HANDLE provided.

**SRS_UAMQP_MESSAGING_09_001: [**The body type of the uAMQP message shall be retrieved using message_get_body_type().**]**
**SRS_UAMQP_MESSAGING_09_002: [**If message_get_body_type() fails, message_create_IoTHubMessage_from_uamqp_message shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_003: [**If the uAMQP message body type is MESSAGE_BODY_TYPE_DATA, the body data shall be treated as binary data.**]**
**SRS_UAMQP_MESSAGING_09_004: [**The uAMQP message body data shall be retrieved using message_get_body_amqp_data().**]**
**SRS_UAMQP_MESSAGING_09_005: [**If message_get_body_amqp_data() fails, message_create_IoTHubMessage_from_uamqp_message shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_006: [**The IOTHUB_MESSAGE instance shall be created using IoTHubMessage_CreateFromByteArray(), passing the uAMQP body bytes as parameter.**]**
**SRS_UAMQP_MESSAGING_09_007: [**If IoTHubMessage_CreateFromByteArray() fails, message_create_IoTHubMessage_from_uamqp_message shall fail and return immediately.**]**

Copying the AMQP-specific properties:
**SRS_UAMQP_MESSAGING_09_008: [**The uAMQP message properties shall be retrieved using message_get_properties().**]**
**SRS_UAMQP_MESSAGING_09_009: [**If message_get_properties() fails, message_create_IoTHubMessage_from_uamqp_message shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_010: [**The message-id property shall be read from the uAMQP message by calling properties_get_message_id.**]**
**SRS_UAMQP_MESSAGING_09_011: [**If the uAMQP message does not contain property `message ID`, it shall be skipped as it is optional.**]**
**SRS_UAMQP_MESSAGING_09_012: [**The type of the message-id property value shall be obtained using amqpvalue_get_type().**]**
**SRS_UAMQP_MESSAGING_09_013: [**If the type of the message-id property value is AMQP_TYPE_NULL, message_create_IoTHubMessage_from_uamqp_message() shall skip processing the message-id (as it is optional) and continue normally.**]**
**SRS_UAMQP_MESSAGING_09_014: [**The message-id value shall be retrieved from the AMQP_VALUE as char sequence**]**
**SRS_UAMQP_MESSAGING_09_015: [**If message-id fails to be obtained, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_016: [**The message-id property shall be set on the IOTHUB_MESSAGE_HANDLE instance by calling IoTHubMessage_SetMessageId(), passing the value read from the uAMQP message.**]**
**SRS_UAMQP_MESSAGING_09_017: [**If IoTHubMessage_SetMessageId fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_018: [**The correlation-id property shall be read from the uAMQP message by calling properties_get_correlation_id.**]**
**SRS_UAMQP_MESSAGING_09_019: [**If the uAMQP message does not contain property `correlation ID`, it shall be skipped as it is optional.**]**
**SRS_UAMQP_MESSAGING_09_020: [**The type of the correlation-id property value shall be obtained using amqpvalue_get_type().**]**
**SRS_UAMQP_MESSAGING_09_021: [**If the type of the correlation-id property value is AMQP_TYPE_NULL, message_create_IoTHubMessage_from_uamqp_message() shall skip processing the correlation-id (as it is optional) and continue normally.**]**
**SRS_UAMQP_MESSAGING_09_022: [**The correlation-id value shall be retrieved from the AMQP_VALUE as char sequence**]**
**SRS_UAMQP_MESSAGING_09_023: [**If correlation-id fails to be obtained, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_024: [**The correlation-id property shall be set on the IOTHUB_MESSAGE_HANDLE by calling IoTHubMessage_SetCorrelationId, passing the value read from the uAMQP message.**]**
**SRS_UAMQP_MESSAGING_09_025: [**If IoTHubMessage_SetCorrelationId fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_100: [**If the uAMQP message contains property `content-type`, it shall be set on IOTHUB_MESSAGE_HANDLE**]**
**SRS_UAMQP_MESSAGING_31_122: [**If the uAMQP message does not contain property `content-type`, it shall be skipped as it is optional**]**
**SRS_UAMQP_MESSAGING_09_101: [**If retrieving the `content-type` property from uAMQP message fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_102: [**If setting the `content-type` property on IOTHUB_MESSAGE_HANDLE fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_103: [**If the uAMQP message contains property `content-encoding`, it shall be set on IOTHUB_MESSAGE_HANDLE**]**
**SRS_UAMQP_MESSAGING_31_123: [**If the uAMQP message does not contain property `content-encoding`, it shall be skipped as it is optional**]
**SRS_UAMQP_MESSAGING_09_104: [**If retrieving the `content-encoding` property from uAMQP message fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_105: [**If setting the `content-encoding` property on IOTHUB_MESSAGE_HANDLE fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_026: [**message_create_IoTHubMessage_from_uamqp_message() shall destroy the uAMQP message properties (obtained with message_get_properties()) by calling properties_destroy().**]**


Copying the AMQP application-properties:
**SRS_UAMQP_MESSAGING_09_027: [**The IOTHUB_MESSAGE_HANDLE properties shall be retrieved using IoTHubMessage_Properties.**]**
**SRS_UAMQP_MESSAGING_09_028: [**If IoTHubMessage_Properties fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_029: [**The uAMQP message application properties shall be retrieved using message_get_application_properties.**]**
**SRS_UAMQP_MESSAGING_09_030: [**If message_get_application_properties fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_031: [**If message_get_application_properties succeeds but returns a NULL application properties map (there are no properties), message_create_IoTHubMessage_from_uamqp_message() shall skip processing the properties and continue normally.**]**
**SRS_UAMQP_MESSAGING_09_032: [**The actual uAMQP message application properties should be extracted from the result of message_get_application_properties using amqpvalue_get_inplace_described_value.**]**
**SRS_UAMQP_MESSAGING_09_033: [**If amqpvalue_get_inplace_described_value fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_034: [**The number of items in the uAMQP message application properties shall be obtained using amqpvalue_get_map_pair_count.**]**
**SRS_UAMQP_MESSAGING_09_035: [**If amqpvalue_get_map_pair_count fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_036: [**message_create_IoTHubMessage_from_uamqp_message() shall iterate through each uAMQP application property and add it to IOTHUB_MESSAGE_HANDLE properties.**]**
**SRS_UAMQP_MESSAGING_09_037: [**The uAMQP application property name and value shall be obtained using amqpvalue_get_map_key_value_pair.**]**
**SRS_UAMQP_MESSAGING_09_038: [**If amqpvalue_get_map_key_value_pair fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_039: [**The uAMQP application property name shall be extracted as string using amqpvalue_get_string.**]**
**SRS_UAMQP_MESSAGING_09_040: [**If amqpvalue_get_string fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_041: [**The uAMQP application property value shall be extracted as string using amqpvalue_get_string.**]**
**SRS_UAMQP_MESSAGING_09_042: [**If amqpvalue_get_string fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_043: [**The application property name and value shall be added to IOTHUB_MESSAGE_HANDLE properties using Map_AddOrUpdate.**]**
**SRS_UAMQP_MESSAGING_09_044: [**If Map_AddOrUpdate fails, message_create_IoTHubMessage_from_uamqp_message() shall fail and return immediately.**]**
**SRS_UAMQP_MESSAGING_09_045: [**message_create_IoTHubMessage_from_uamqp_message() shall destroy the uAMQP message property name and value (obtained with amqpvalue_get_string) by calling amqpvalue_destroy().**]**
**SRS_UAMQP_MESSAGING_09_046: [**message_create_IoTHubMessage_from_uamqp_message() shall destroy the uAMQP message property (obtained with message_get_application_properties) by calling amqpvalue_destroy().**]**


### message_create_uamqp_encoding_from_iothub_message

Creates a binary blob containing AMQP encoding of the same message defined by the IOTHUB_MESSAGE_HANDLE provided.

**SRS_UAMQP_MESSAGING_31_112: [**If optional message-id is present in the message, encode it into the AMQP message.**]**
**SRS_UAMQP_MESSAGING_31_113: [**If optional correlation-id is present in the message, encode it into the AMQP message.**]**
**SRS_UAMQP_MESSAGING_31_114: [**If optional content-type is present in the message, encode it into the AMQP message.**]**
**SRS_UAMQP_MESSAGING_31_115: [**If optional content-encoding is present in the message, encode it into the AMQP message.**]**
**SRS_UAMQP_MESSAGING_31_116: [**Gets message properties associated with the IOTHUB_MESSAGE_HANDLE to encode, returning the properties and their encoded length.**]**
**SRS_UAMQP_MESSAGING_31_117: [**Get application message properties associated with the IOTHUB_MESSAGE_HANDLE to encode, returning the properties and their encoded length.**]**
**SRS_UAMQP_MESSAGING_31_118: [**Gets data associated with IOTHUB_MESSAGE_HANDLE to encode, either from underlying byte array or string format.**]**
**SRS_UAMQP_MESSAGING_31_119: [**Invoke underlying AMQP encode routines on data waiting to be encoded.  .**]**
**SRS_UAMQP_MESSAGING_31_120: [**Create a blob that contains AMQP encoding of IOTHUB_MESSAGE_HANDLE.**]**
**SRS_UAMQP_MESSAGING_31_121: [**Any errors during `message_create_uamqp_encoding_from_iothub_message` stop processing on this message.**]**
**SRS_UAMQP_MESSAGING_32_001: [**If optional diagnostic properties are present in the iot hub message, encode them into the AMQP message as annotation properties: `Diagnostic-Id` `Correlation-Context`.**]**
**SRS_UAMQP_MESSAGING_32_002: [**If optional diagnostic properties are not present in the iot hub message, no error should happen.**]**

