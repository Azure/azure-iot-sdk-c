// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_uamqp_c/link.h"
#include "azure_uamqp_c/messaging.h"
#include "azure_uamqp_c/message_sender.h"
#include "azure_uamqp_c/message_receiver.h"
#include "uamqp_messaging.h"
#include "iothub_client_private.h"
#include "iothub_client_version.h"
#include "iothubtransport_amqp_messenger.h"

#define RESULT_OK 0
#define INDEFINITE_TIME ((time_t)(-1))

#define IOTHUB_DEVICES_PATH_FMT                "%s/devices/%s"
#define IOTHUB_EVENT_SEND_ADDRESS_FMT          "amqps://%s/messages/events"
#define IOTHUB_MESSAGE_RECEIVE_ADDRESS_FMT     "amqps://%s/messages/devicebound"
#define MESSAGE_SENDER_LINK_NAME_PREFIX        "link-snd"
#define MESSAGE_SENDER_MAX_LINK_SIZE           UINT64_MAX
#define MESSAGE_RECEIVER_LINK_NAME_PREFIX      "link-rcv"
#define MESSAGE_RECEIVER_MAX_LINK_SIZE         65536
#define DEFAULT_EVENT_SEND_RETRY_LIMIT         100
#define UNIQUE_ID_BUFFER_SIZE                  37
#define STRING_NULL_TERMINATOR                 '\0'
 
typedef struct MESSENGER_INSTANCE_TAG
{
	STRING_HANDLE device_id;
	STRING_HANDLE iothub_host_fqdn;
	PDLIST_ENTRY wait_to_send_list;
	DLIST_ENTRY in_progress_list;
	MESSENGER_STATE state;
	
	ON_MESSENGER_STATE_CHANGED_CALLBACK on_state_changed_callback;
	void* on_state_changed_context;

	bool receive_messages;
	ON_NEW_MESSAGE_RECEIVED on_message_received_callback;
	void* on_message_received_context;

	SESSION_HANDLE session_handle;
	LINK_HANDLE sender_link;
	MESSAGE_SENDER_HANDLE message_sender;
	LINK_HANDLE receiver_link;
	MESSAGE_RECEIVER_HANDLE message_receiver;

	size_t event_send_retry_limit;
	size_t event_send_error_count;
} MESSENGER_INSTANCE;


static STRING_HANDLE create_devices_path(STRING_HANDLE iothub_host_fqdn, STRING_HANDLE device_id)
{
	STRING_HANDLE devices_path;

	if ((devices_path = STRING_new()) == NULL)
	{
		LogError("Failed creating devices_path (STRING_new failed)");
	}
	else if (STRING_sprintf(devices_path, IOTHUB_DEVICES_PATH_FMT, STRING_c_str(iothub_host_fqdn), STRING_c_str(device_id)) != RESULT_OK)
	{
		STRING_delete(devices_path);
		devices_path = NULL;
		LogError("Failed creating devices_path (STRING_sprintf failed)");
	}

	return devices_path;
}

static STRING_HANDLE create_event_send_address(STRING_HANDLE devices_path)
{
	STRING_HANDLE event_send_address;

	if ((event_send_address = STRING_new()) == NULL)
	{
		LogError("Failed creating the event_send_address (STRING_new failed)");
	}
	else if (STRING_sprintf(event_send_address, IOTHUB_EVENT_SEND_ADDRESS_FMT, STRING_c_str(devices_path)) != RESULT_OK)
	{
		STRING_delete(event_send_address);
		event_send_address = NULL;
		LogError("Failed creating the event_send_address (STRING_sprintf failed)");
	}

	return event_send_address;
}

static STRING_HANDLE create_event_sender_source_name(STRING_HANDLE link_name)
{
	STRING_HANDLE source_name;
	
	if ((source_name = STRING_new()) == NULL)
	{
		LogError("Failed creating the source_name (STRING_new failed)");
	}
	else if (STRING_sprintf(source_name, "%s-source", STRING_c_str(link_name)) != RESULT_OK)
	{
		STRING_delete(source_name);
		source_name = NULL;
		LogError("Failed creating the source_name (STRING_sprintf failed)");
	}

	return source_name;
}

static STRING_HANDLE create_message_receive_address(STRING_HANDLE devices_path)
{
	STRING_HANDLE message_receive_address;

	if ((message_receive_address = STRING_new()) == NULL)
	{
		LogError("Failed creating the message_receive_address (STRING_new failed)");
	}
	else if (STRING_sprintf(message_receive_address, IOTHUB_MESSAGE_RECEIVE_ADDRESS_FMT, STRING_c_str(devices_path)) != RESULT_OK)
	{
		STRING_delete(message_receive_address);
		message_receive_address = NULL;
		LogError("Failed creating the message_receive_address (STRING_sprintf failed)");
	}

	return message_receive_address;
}

static STRING_HANDLE create_message_receiver_target_name(STRING_HANDLE link_name)
{
	STRING_HANDLE target_name;

	if ((target_name = STRING_new()) == NULL)
	{
		LogError("Failed creating the target_name (STRING_new failed)");
	}
	else if (STRING_sprintf(target_name, "%s-target", STRING_c_str(link_name)) != RESULT_OK)
	{
		STRING_delete(target_name);
		target_name = NULL;
		LogError("Failed creating the target_name (STRING_sprintf failed)");
	}

	return target_name;
}

static STRING_HANDLE create_link_name(const char* prefix, const char* infix)
{
	char* unique_id;
	STRING_HANDLE tag = NULL;

	if ((unique_id = (char*)malloc(sizeof(char) * UNIQUE_ID_BUFFER_SIZE + 1)) == NULL)
	{
		LogError("Failed generating an unique tag (malloc failed)");
	}
	else
	{
        memset(unique_id, 0, sizeof(char) * UNIQUE_ID_BUFFER_SIZE + 1);

		if (UniqueId_Generate(unique_id, UNIQUE_ID_BUFFER_SIZE) != UNIQUEID_OK)
		{
			LogError("Failed generating an unique tag (UniqueId_Generate failed)");
		}
		else if ((tag = STRING_new()) == NULL)
		{
			LogError("Failed generating an unique tag (STRING_new failed)");
		}
		else if (STRING_sprintf(tag, "%s-%s-%s", prefix, infix, unique_id) != RESULT_OK)
		{
			STRING_delete(tag);
			tag = NULL;
			LogError("Failed generating an unique tag (STRING_sprintf failed)");
		}

		free(unique_id);
	}

	return tag;
}

static void update_state(MESSENGER_INSTANCE* instance, MESSENGER_STATE new_state)
{
	if (new_state != instance->state)
	{
		MESSENGER_STATE previous_state = instance->state;
		instance->state = new_state;

		if (instance->on_state_changed_callback != NULL)
		{
			instance->on_state_changed_callback(instance->on_state_changed_context, previous_state, new_state);
		}
	}
}

static void attach_device_client_type_to_link(LINK_HANDLE link)
{
	fields attach_properties;
	AMQP_VALUE device_client_type_key_name;
	AMQP_VALUE device_client_type_value;
	int result;

	if ((attach_properties = amqpvalue_create_map()) == NULL)
	{
		LogError("Failed to create the map for device client type.");
	}
	else
	{
		if ((device_client_type_key_name = amqpvalue_create_symbol("com.microsoft:client-version")) == NULL)
		{
			LogError("Failed to create the key name for the device client type.");
		}
		else
		{
			if ((device_client_type_value = amqpvalue_create_string(CLIENT_DEVICE_TYPE_PREFIX CLIENT_DEVICE_BACKSLASH IOTHUB_SDK_VERSION)) == NULL)
			{
				LogError("Failed to create the key value for the device client type.");
			}
			else
			{
				if ((result = amqpvalue_set_map_value(attach_properties, device_client_type_key_name, device_client_type_value)) != 0)
				{
					LogError("Failed to set the property map for the device client type (error code is: %d)", result);
				}
				else if ((result = link_set_attach_properties(link, attach_properties)) != 0)
				{
					LogError("Unable to attach the device client type to the link properties (error code is: %d)", result);
				}

				amqpvalue_destroy(device_client_type_value);
			}

			amqpvalue_destroy(device_client_type_key_name);
		}

		amqpvalue_destroy(attach_properties);
	}
}

static void destroy_event_sender(MESSENGER_INSTANCE* instance)
{
	if (instance->message_sender != NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_060: [`instance->message_sender` shall be destroyed using messagesender_destroy()]
		messagesender_destroy(instance->message_sender);
		instance->message_sender = NULL;
	}

	if (instance->sender_link != NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_063: [`instance->sender_link` shall be destroyed using link_destroy()]
		link_destroy(instance->sender_link);
		instance->sender_link = NULL;
	}
}

static void on_event_sender_state_changed_callback(void* context, MESSAGE_SENDER_STATE new_state, MESSAGE_SENDER_STATE previous_state)
{
	(void)previous_state;

	if (context != NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_118: [If the messagesender new state is MESSAGE_SENDER_STATE_OPEN, `instance->state` shall be set to MESSENGER_STATE_STARTED, and `instance->on_state_changed_callback` invoked if provided]
		if (new_state == MESSAGE_SENDER_STATE_OPEN)
		{
			update_state((MESSENGER_INSTANCE*)context, MESSENGER_STATE_STARTED);
		}
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_119: [If the messagesender new state is MESSAGE_SENDER_STATE_ERROR, `instance->state` shall be set to MESSENGER_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided]
		else if (new_state == MESSAGE_SENDER_STATE_ERROR)
		{
			update_state((MESSENGER_INSTANCE*)context, MESSENGER_STATE_ERROR);
		}
	}
}

static int create_event_sender(MESSENGER_INSTANCE* instance)
{
	int result;

	STRING_HANDLE link_name = NULL;
	STRING_HANDLE source_name = NULL;
	AMQP_VALUE source = NULL;
	AMQP_VALUE target = NULL;
	STRING_HANDLE devices_path = NULL;
	STRING_HANDLE event_send_address = NULL;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_033: [A variable, named `devices_path`, shall be created concatenating `instance->iothub_host_fqdn`, "/devices/" and `instance->device_id`]
	if ((devices_path = create_devices_path(instance->iothub_host_fqdn, instance->device_id)) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_034: [If `devices_path` fails to be created, messenger_start() shall fail and return __LINE__]
		result = __LINE__;
		LogError("Failed creating the message sender (failed creating the 'devices_path')");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_035: [A variable, named `event_send_address`, shall be created concatenating "amqps://", `devices_path` and "/messages/events"]
	else if ((event_send_address = create_event_send_address(devices_path)) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_036: [If `event_send_address` fails to be created, messenger_start() shall fail and return __LINE__]
		result = __LINE__;
		LogError("Failed creating the message sender (failed creating the 'event_send_address')");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_037: [A `link_name` variable shall be created using an unique string label per AMQP session]
	else if ((link_name = create_link_name(MESSAGE_SENDER_LINK_NAME_PREFIX, STRING_c_str(instance->device_id))) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_038: [If `link_name` fails to be created, messenger_start() shall fail and return __LINE__]
		result = __LINE__;
		LogError("Failed creating the message sender (failed creating an unique link name)");
	}
	else if ((source_name = create_event_sender_source_name(link_name)) == NULL)
	{
		result = __LINE__;
		LogError("Failed creating the message sender (failed creating an unique source name)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_039: [A `source` variable shall be created with messaging_create_source() using an unique string label per AMQP session]
	else if ((source = messaging_create_source(STRING_c_str(source_name))) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_040: [If `source` fails to be created, messenger_start() shall fail and return __LINE__]
		result = __LINE__;
		LogError("Failed creating the message sender (messaging_create_source failed)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_041: [A `target` variable shall be created with messaging_create_target() using `event_send_address`]
	else if ((target = messaging_create_target(STRING_c_str(event_send_address))) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_042: [If `target` fails to be created, messenger_start() shall fail and return __LINE__]
		result = __LINE__;
		LogError("Failed creating the message sender (messaging_create_target failed)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_043: [`instance->sender_link` shall be set using link_create(), passing `instance->session_handle`, `link_name`, "role_sender", `source` and `target` as parameters]
	else if ((instance->sender_link = link_create(instance->session_handle, STRING_c_str(link_name), role_sender, source, target)) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_044: [If link_create() fails, messenger_start() shall fail and return __LINE__]
		result = __LINE__;
		LogError("Failed creating the message sender (link_create failed)");
	}
	else
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_047: [`instance->sender_link` maximum message size shall be set to UINT64_MAX using link_set_max_message_size()]
		if (link_set_max_message_size(instance->sender_link, MESSAGE_SENDER_MAX_LINK_SIZE) != RESULT_OK)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_048: [If link_set_max_message_size() fails, it shall be logged and ignored.]
			LogError("Failed setting message sender link max message size.");
		}

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_049: [`instance->sender_link` should have a property "com.microsoft:client-version" set as `CLIENT_DEVICE_TYPE_PREFIX/IOTHUB_SDK_VERSION`, using amqpvalue_set_map_value() and link_set_attach_properties()]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_050: [If amqpvalue_set_map_value() or link_set_attach_properties() fail, the failure shall be ignored]
		attach_device_client_type_to_link(instance->sender_link);

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_051: [`instance->message_sender` shall be created using messagesender_create(), passing the `instance->sender_link` and `on_event_sender_state_changed_callback`]
		if ((instance->message_sender = messagesender_create(instance->sender_link, on_event_sender_state_changed_callback, (void*)instance)) == NULL)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_052: [If messagesender_create() fails, messenger_start() shall fail and return __LINE__]
			result = __LINE__;
			link_destroy(instance->sender_link);
			instance->sender_link = NULL;
			LogError("Failed creating the message sender (messagesender_create failed)");
		}
		else
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_053: [`instance->message_sender` shall be opened using messagesender_open()]
			if (messagesender_open(instance->message_sender) != RESULT_OK)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_054: [If messagesender_open() fails, messenger_start() shall fail and return __LINE__]
				result = __LINE__;
				messagesender_destroy(instance->message_sender);
				instance->message_sender = NULL;
				link_destroy(instance->sender_link);
				instance->sender_link = NULL;
				LogError("Failed opening the AMQP message sender.");
			}
			else
			{
				result = RESULT_OK;
			}
		}
	}

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_055: [Before returning, messenger_start() shall release all the temporary memory it has allocated]
	if (link_name != NULL)
		STRING_delete(link_name);
	if (source_name != NULL)
		STRING_delete(source_name);
	if (source != NULL)
		amqpvalue_destroy(source);
	if (target != NULL)
		amqpvalue_destroy(target);
	if (devices_path != NULL)
		STRING_delete(devices_path);
	if (event_send_address != NULL)
		STRING_delete(event_send_address);

	return result;
}

static void destroy_message_receiver(MESSENGER_INSTANCE* instance)
{
	if (instance->message_receiver != NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_061: [`instance->message_receiver` shall be closed using messagereceiver_close()]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_093: [`instance->message_receiver` shall be closed using messagereceiver_close()]
		if (messagereceiver_close(instance->message_receiver) != RESULT_OK)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_094: [If messagereceiver_close() fails, it shall be logged and ignored]
			LogError("Failed closing the AMQP message receiver (this failure will be ignored).");
		}

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_062: [`instance->message_receiver` shall be destroyed using messagereceiver_destroy()]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_095: [`instance->message_receiver` shall be destroyed using messagereceiver_destroy()]
		messagereceiver_destroy(instance->message_receiver);

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_096: [`instance->message_receiver` shall be set to NULL]
		instance->message_receiver = NULL;
	}

	if (instance->receiver_link != NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_064: [`instance->receiver_link` shall be destroyed using link_destroy()]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_097: [`instance->receiver_link` shall be destroyed using link_destroy()]
		link_destroy(instance->receiver_link);
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_098: [`instance->receiver_link` shall be set to NULL]
		instance->receiver_link = NULL;
	}
}

static void on_message_receiver_state_changed_callback(const void* context, MESSAGE_RECEIVER_STATE new_state, MESSAGE_RECEIVER_STATE previous_state)
{
	(void)previous_state;

	if (context != NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_120: [If the messagereceiver new state is MESSAGE_RECEIVER_STATE_ERROR, `instance->state` shall be set to MESSENGER_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided]
		if (new_state == MESSAGE_RECEIVER_STATE_ERROR)
		{
			update_state((MESSENGER_INSTANCE*)context, MESSENGER_STATE_ERROR);
		}
	}
}

static AMQP_VALUE on_message_received_internal_callback(const void* context, MESSAGE_HANDLE message)
{
	AMQP_VALUE result = NULL;
	int api_call_result;
	IOTHUB_MESSAGE_HANDLE iothub_message = NULL;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_121: [An IOTHUB_MESSAGE_HANDLE shall be obtained from MESSAGE_HANDLE using IoTHubMessage_CreateFromUamqpMessage()]
	if ((api_call_result = IoTHubMessage_CreateFromUamqpMessage(message, &iothub_message)) != RESULT_OK)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_122: [If IoTHubMessage_CreateFromUamqpMessage() fails, on_message_received_internal_callback shall return the result of messaging_delivery_rejected()]
		result = messaging_delivery_rejected("Rejected due to failure reading AMQP message", "Failed reading AMQP message");

		LogError("on_message_received_internal_callback failed (IoTHubMessage_CreateFromUamqpMessage; error = %d).", api_call_result);
	}
	else
	{
		MESSENGER_INSTANCE* instance = (MESSENGER_INSTANCE*)context;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_123: [`instance->on_message_received_callback` shall be invoked passing the IOTHUB_MESSAGE_HANDLE]
		MESSENGER_DISPOSITION_RESULT disposition_result = instance->on_message_received_callback(instance->on_message_received_context, iothub_message);

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_124: [The IOTHUB_MESSAGE_HANDLE instance shall be destroyed using IoTHubMessage_Destroy()]
		IoTHubMessage_Destroy(iothub_message);

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_125: [If `instance->on_message_received_callback` returns MESSENGER_DISPOSITION_RESULT_ACCEPTED, on_message_received_internal_callback shall return the result of messaging_delivery_accepted()]
		if (disposition_result == MESSENGER_DISPOSITION_RESULT_ACCEPTED)
		{
			result = messaging_delivery_accepted();
		}
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_126: [If `instance->on_message_received_callback` returns MESSENGER_DISPOSITION_RESULT_ABANDONED, on_message_received_internal_callback shall return the result of messaging_delivery_released()]
		else if (disposition_result == MESSENGER_DISPOSITION_RESULT_ABANDONED)
		{
			result = messaging_delivery_released();
		}
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_127: [If `instance->on_message_received_callback` returns MESSENGER_DISPOSITION_RESULT_REJECTED, on_message_received_internal_callback shall return the result of messaging_delivery_rejected()]
		else if (disposition_result == MESSENGER_DISPOSITION_RESULT_REJECTED)
		{
			result = messaging_delivery_rejected("Rejected by application", "Rejected by application");
		}
	}

	return result;
}

static int create_message_receiver(MESSENGER_INSTANCE* instance)
{
	int result;

	STRING_HANDLE devices_path = NULL;
	STRING_HANDLE message_receive_address = NULL;
	STRING_HANDLE link_name = NULL;
	STRING_HANDLE target_name = NULL;
	AMQP_VALUE source = NULL;
	AMQP_VALUE target = NULL;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_068: [A variable, named `devices_path`, shall be created concatenating `instance->iothub_host_fqdn`, "/devices/" and `instance->device_id`]
	if ((devices_path = create_devices_path(instance->iothub_host_fqdn, instance->device_id)) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_069: [If `devices_path` fails to be created, messenger_do_work() shall fail and return]
		result = __LINE__;
		LogError("Failed creating the message receiver (failed creating the 'devices_path')");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_070: [A variable, named `message_receive_address`, shall be created concatenating "amqps://", `devices_path` and "/messages/devicebound"]
	else if ((message_receive_address = create_message_receive_address(devices_path)) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_071: [If `message_receive_address` fails to be created, messenger_do_work() shall fail and return]
		result = __LINE__;
		LogError("Failed creating the message receiver (failed creating the 'message_receive_address')");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_072: [A `link_name` variable shall be created using an unique string label per AMQP session]
	else if ((link_name = create_link_name(MESSAGE_RECEIVER_LINK_NAME_PREFIX, STRING_c_str(instance->device_id))) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_073: [If `link_name` fails to be created, messenger_start() shall fail and return __LINE__]
		result = __LINE__;
		LogError("Failed creating the message receiver (failed creating an unique link name)");
	}
	else if ((target_name = create_message_receiver_target_name(link_name)) == NULL)
	{
		result = __LINE__;
		LogError("Failed creating the message receiver (failed creating an unique target name)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_074: [A `target` variable shall be created with messaging_create_target() using an unique string label per AMQP session]
	else if ((target = messaging_create_target(STRING_c_str(target_name))) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_075: [If `target` fails to be created, messenger_start() shall fail and return __LINE__]
		result = __LINE__;
		LogError("Failed creating the message receiver (messaging_create_target failed)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_076: [A `source` variable shall be created with messaging_create_source() using `message_receive_address`]
	else if ((source = messaging_create_source(STRING_c_str(message_receive_address))) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_077: [If `source` fails to be created, messenger_start() shall fail and return __LINE__]
		result = __LINE__;
		LogError("Failed creating the message receiver (messaging_create_source failed)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_078: [`instance->receiver_link` shall be set using link_create(), passing `instance->session_handle`, `link_name`, "role_receiver", `source` and `target` as parameters]
	else if ((instance->receiver_link = link_create(instance->session_handle, STRING_c_str(link_name), role_receiver, source, target)) == NULL)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_079: [If link_create() fails, messenger_do_work() shall fail and return]
		result = __LINE__;
		LogError("Failed creating the message receiver (link_create failed)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_080: [`instance->receiver_link` settle mode shall be set to "receiver_settle_mode_first" using link_set_rcv_settle_mode(), ]
	else if (link_set_rcv_settle_mode(instance->receiver_link, receiver_settle_mode_first) != RESULT_OK)
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_081: [If link_set_rcv_settle_mode() fails, messenger_do_work() shall fail and return]
		result = __LINE__;
		LogError("Failed creating the message receiver (link_set_rcv_settle_mode failed)");
	}
	else
	{
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_082: [`instance->receiver_link` maximum message size shall be set to 65536 using link_set_max_message_size()]
		if (link_set_max_message_size(instance->receiver_link, MESSAGE_RECEIVER_MAX_LINK_SIZE) != RESULT_OK)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_083: [If link_set_max_message_size() fails, it shall be logged and ignored.]
			LogError("Failed setting message receiver link max message size.");
		}

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_084: [`instance->receiver_link` should have a property "com.microsoft:client-version" set as `CLIENT_DEVICE_TYPE_PREFIX/IOTHUB_SDK_VERSION`, using amqpvalue_set_map_value() and link_set_attach_properties()]
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_085: [If amqpvalue_set_map_value() or link_set_attach_properties() fail, the failure shall be ignored]
		attach_device_client_type_to_link(instance->receiver_link);

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_086: [`instance->message_receiver` shall be created using messagereceiver_create(), passing the `instance->receiver_link` and `on_messagereceiver_state_changed_callback`]
		if ((instance->message_receiver = messagereceiver_create(instance->receiver_link, on_message_receiver_state_changed_callback, (void*)instance)) == NULL)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_087: [If messagereceiver_create() fails, messenger_do_work() shall fail and return]
			result = __LINE__;
			link_destroy(instance->receiver_link);
			LogError("Failed creating the message receiver (messagereceiver_create failed)");
		}
		else
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_088: [`instance->message_receiver` shall be opened using messagereceiver_open(), passing `on_message_received_internal_callback`]
			if (messagereceiver_open(instance->message_receiver, on_message_received_internal_callback, (const void*)instance) != RESULT_OK)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_089: [If messagereceiver_open() fails, messenger_do_work() shall fail and return]
				result = __LINE__;
				messagereceiver_destroy(instance->message_receiver);
				link_destroy(instance->receiver_link);
				LogError("Failed opening the AMQP message receiver.");
			}
			else
			{
				result = RESULT_OK;
			}
		}
	}

	if (devices_path != NULL)
		STRING_delete(devices_path);
	if (message_receive_address != NULL)
		STRING_delete(message_receive_address);
	if (link_name != NULL)
		STRING_delete(link_name);
	if (target_name != NULL)
		STRING_delete(target_name);
	if (source != NULL)
		amqpvalue_destroy(source);
	if (target != NULL)
		amqpvalue_destroy(target);

	return result;
}

static void move_event_to_in_progress_list(IOTHUB_MESSAGE_LIST* message, MESSENGER_INSTANCE* instance)
{
	DList_RemoveEntryList(&message->entry);
	DList_InsertTailList(&instance->in_progress_list, &message->entry);
}

static int is_event_in_in_progress_list(IOTHUB_MESSAGE_LIST* message)
{
	return !DList_IsListEmpty(&message->entry);
}

static void remove_event_from_in_progress_list(IOTHUB_MESSAGE_LIST* message)
{
	DList_RemoveEntryList(&message->entry);
	DList_InitializeListHead(&message->entry);
}

static void move_event_to_wait_to_send_list(IOTHUB_MESSAGE_LIST* message, MESSENGER_INSTANCE* instance)
{
	remove_event_from_in_progress_list(message);
	DList_InsertTailList(instance->wait_to_send_list, &message->entry);
}

static void move_events_to_wait_to_send_list(MESSENGER_INSTANCE* instance)
{
	PDLIST_ENTRY entry = instance->in_progress_list.Blink;

	while (entry != &instance->in_progress_list)
	{
		IOTHUB_MESSAGE_LIST* message = containingRecord(entry, IOTHUB_MESSAGE_LIST, entry);
		entry = entry->Blink;
		move_event_to_wait_to_send_list(message, instance);
	}
}

static void on_message_send_complete_callback(void* context, MESSAGE_SEND_RESULT send_result)
{
	if (context != NULL)
	{
		IOTHUB_MESSAGE_LIST* message = (IOTHUB_MESSAGE_LIST*)context;

		if (message->callback != NULL)
		{
			IOTHUB_CLIENT_RESULT iot_hub_send_result;

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_107: [If no failure occurs, `MESSAGE_HANDLE->callback` shall be invoked with result IOTHUB_CLIENT_CONFIRMATION_OK]
			if (send_result == MESSAGE_SEND_OK)
			{
				iot_hub_send_result = IOTHUB_CLIENT_CONFIRMATION_OK;
			}
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_108: [If a failure occurred, `MESSAGE_HANDLE->callback` shall be invoked with result IOTHUB_CLIENT_CONFIRMATION_ERROR]
			else
			{
				iot_hub_send_result = IOTHUB_CLIENT_CONFIRMATION_ERROR;
			}

			message->callback(iot_hub_send_result, message->context);
		}
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_128: [MESSAGE_HANDLE shall be removed from `instance->in_progress_list`]
		if (is_event_in_in_progress_list(message))
		{
			remove_event_from_in_progress_list(message);
		}

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_129: [`MESSAGE_HANDLE->messageHandle` shall be destroyed using IoTHubMessage_Destroy()]
		IoTHubMessage_Destroy(message->messageHandle);
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_130: [MESSAGE_HANDLE shall be destroyed using free()]
		free(message);
	}
}

static IOTHUB_MESSAGE_LIST* get_next_event_to_send(MESSENGER_INSTANCE* instance)
{
	IOTHUB_MESSAGE_LIST* message;

	if (DList_IsListEmpty(instance->wait_to_send_list))
	{
		message = NULL;
	}
	else
	{
		PDLIST_ENTRY list_entry = instance->wait_to_send_list->Flink;
		message = containingRecord(list_entry, IOTHUB_MESSAGE_LIST, entry);
	}

	return message;
}

static int send_pending_events(MESSENGER_INSTANCE* instance)
{
	int result = RESULT_OK;
	int api_result;

	IOTHUB_MESSAGE_LIST* message;

	while (result == RESULT_OK && (message = get_next_event_to_send(instance)) != NULL)
	{
		MESSAGE_HANDLE amqp_message = NULL;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_100: [messenger_do_work() shall move each event to be sent from `instance->wait_to_send_list` to `instance->in_progress_list`]
		move_event_to_in_progress_list(message, instance);

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_101: [A MESSAGE_HANDLE shall be obtained out of the event's IOTHUB_MESSAGE_HANDLE instance by using message_create_from_iothub_message()]
		if ((api_result = message_create_from_iothub_message(message->messageHandle, &amqp_message)) != RESULT_OK)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_102: [If message_create_from_iothub_message() fails, `MESSAGE_HANDLE->callback` shall be invoked, if defined, with result IOTHUB_CLIENT_CONFIRMATION_ERROR]
			on_message_send_complete_callback(message, MESSAGE_SEND_ERROR);

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_103: [If message_create_from_iothub_message() fails, messenger_do_work() shall fail and return]
			LogError("Failed sending event message (failed creating AMQP message; error: %d).", api_result);
		}
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_104: [The MESSAGE_HANDLE shall be submitted for sending using messagesender_send(), passing `on_message_send_complete_callback`]
		else if ((api_result = messagesender_send(instance->message_sender, amqp_message, on_message_send_complete_callback, message)) != RESULT_OK)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_105: [If messagesender_send() fails, the event shall be rolled back from `instance->in_progress_list` to `instance->wait_to_send_list`]
			result = __LINE__;

			LogError("Failed sending event (messagesender_send failed; error: %d)", api_result);

			move_event_to_wait_to_send_list(message, instance);
		}

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_106: [The MESSAGE_HANDLE shall be destroyed using message_destroy().]
		if (amqp_message != NULL)
			message_destroy(amqp_message);
	}

	return result;
}


// Public API:

int messenger_subscribe_for_messages(MESSENGER_HANDLE messenger_handle, ON_NEW_MESSAGE_RECEIVED on_message_received_callback, void* context)
{
	int result;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_016: [If `messenger_handle` is NULL, messenger_subscribe_for_messages() shall fail and return __LINE__]
	if (messenger_handle == NULL)
	{
		result = __LINE__;
		LogError("messenger_subscribe_for_messages failed (messenger_handle is NULL)");
	}
	else
	{
		MESSENGER_INSTANCE* instance = (MESSENGER_INSTANCE*)messenger_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_017: [If `instance->receive_messages` is already true, messenger_subscribe_for_messages() shall fail and return __LINE__]
		if (instance->receive_messages)
		{
			result = __LINE__;
			LogError("messenger_subscribe_for_messages failed (messenger already subscribed)");
		}
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_018: [If `on_message_received_callback` is NULL, messenger_subscribe_for_messages() shall fail and return __LINE__]
		else if (on_message_received_callback == NULL)
		{
			result = __LINE__;
			LogError("messenger_subscribe_for_messages failed (on_message_received_callback is NULL)");
		}
		else
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_019: [`on_message_received_callback` shall be saved on `instance->on_message_received_callback`]
			instance->on_message_received_callback = on_message_received_callback;

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_020: [`context` shall be saved on `instance->on_message_received_context`]
			instance->on_message_received_context = context;

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_021: [messenger_subscribe_for_messages() shall set `instance->receive_messages` to true]
			instance->receive_messages = true;

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_022: [If no failures occurr, messenger_subscribe_for_messages() shall return 0]
			result = RESULT_OK;
		}
	}

	return result;
}

int messenger_unsubscribe_for_messages(MESSENGER_HANDLE messenger_handle)
{
	int result;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_023: [If `messenger_handle` is NULL, messenger_unsubscribe_for_messages() shall fail and return __LINE__]
	if (messenger_handle == NULL)
	{
		result = __LINE__;
		LogError("messenger_unsubscribe_for_messages failed (messenger_handle is NULL)");
	}
	else
	{
		MESSENGER_INSTANCE* instance = (MESSENGER_INSTANCE*)messenger_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_024: [If `instance->receive_messages` is already false, messenger_unsubscribe_for_messages() shall fail and return __LINE__]
		if (instance->receive_messages == false)
		{
			result = __LINE__;
			LogError("messenger_unsubscribe_for_messages failed (messenger is not subscribed)");
		}
		else
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_025: [messenger_unsubscribe_for_messages() shall set `instance->receive_messages` to false]
			instance->receive_messages = false;
			
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_026: [messenger_unsubscribe_for_messages() shall set `instance->on_message_received_callback` to NULL]
			instance->on_message_received_callback = NULL;
			
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_027: [messenger_unsubscribe_for_messages() shall set `instance->on_message_received_context` to NULL]
			instance->on_message_received_context = NULL;
			
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_028: [If no failures occurr, messenger_unsubscribe_for_messages() shall return 0]
			result = RESULT_OK;
		}
	}

	return result;
}

int messenger_start(MESSENGER_HANDLE messenger_handle, SESSION_HANDLE session_handle)
{
	int result;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_029: [If `messenger_handle` is NULL, messenger_start() shall fail and return __LINE__]
	if (messenger_handle == NULL)
	{
		result = __LINE__;
		LogError("messenger_start failed (messenger_handle is NULL)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_030: [If `session_handle` is NULL, messenger_start() shall fail and return __LINE__]
	else if (session_handle == NULL)
	{
		result = __LINE__;
		LogError("messenger_start failed (session_handle is NULL)");
	}
	else
	{
		MESSENGER_INSTANCE* instance = (MESSENGER_INSTANCE*)messenger_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_031: [If `instance->state` is not MESSENGER_STATE_STOPPED, messenger_start() shall fail and return __LINE__]
		if (instance->state != MESSENGER_STATE_STOPPED)
		{
			result = __LINE__;
			LogError("messenger_start failed (current state is %d; expected MESSENGER_STATE_STOPPED)", instance->state);
		}
		else
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_032: [`session_handle` shall be saved on `instance->session_handle`, and the `instance` marked as started]
			instance->session_handle = session_handle;

			if (create_event_sender(instance) != RESULT_OK)
			{
				result = __LINE__;
				LogError("messenger_start failed (failed creating the messagesender)");
			}
			else
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_115: [If no failures occurr, `instance->state` shall be set to MESSENGER_STATE_STARTING, and `instance->on_state_changed_callback` invoked if provided]
				instance->state = MESSENGER_STATE_STARTING;

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_056: [If no failures occurr, messenger_start() shall return 0]
				result = RESULT_OK;
			}
		}
	}

	return result;
}

int messenger_stop(MESSENGER_HANDLE messenger_handle)
{
	int result;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_057: [If `messenger_handle` is NULL, messenger_stop() shall fail and return __LINE__]
	if (messenger_handle == NULL)
	{
		result = __LINE__;
		LogError("messenger_stop failed (messenger_handle is NULL)");
	}
	else
	{
		MESSENGER_INSTANCE* instance = (MESSENGER_INSTANCE*)messenger_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_058: [If `instance->state` is MESSENGER_STATE_STOPPED, messenger_start() shall fail and return __LINE__]
		if (instance->state == MESSENGER_STATE_STOPPED)
		{
			result = __LINE__;
			LogError("messenger_stop failed (messenger is already stopped)");
		}
		else
		{
			destroy_event_sender(instance);
			destroy_message_receiver(instance);

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_116: [`instance->state` shall be set to MESSENGER_STATE_STOPPED, and `instance->on_state_changed_callback` invoked if provided]
			update_state(instance, MESSENGER_STATE_STOPPED);

			result = RESULT_OK;
		}
	}

	return result;
}

void messenger_do_work(MESSENGER_HANDLE messenger_handle)
{
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_065: [If `messenger_handle` is NULL, messenger_do_work() shall fail and return]
	if (messenger_handle == NULL)
	{
		LogError("messenger_do_work failed (messenger_handle is NULL)");
	}
	else
	{
		MESSENGER_INSTANCE* instance = (MESSENGER_INSTANCE*)messenger_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_066: [If `instance->state` is not MESSENGER_STATE_STARTED, messenger_do_work() shall return]
		if (instance->state == MESSENGER_STATE_STARTED)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_067: [If `instance->receive_messages` is true and `instance->message_receiver` is NULL, a message_receiver shall be created]
			if (instance->receive_messages == true &&
				instance->message_receiver == NULL &&
				create_message_receiver(instance) != RESULT_OK)
			{
				LogError("messenger_do_work warning (failed creating the message receiver [%s])", STRING_c_str(instance->device_id));
			}
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_092: [If `instance->receive_messages` is false and `instance->message_receiver` is not NULL, it shall be destroyed]
			else if (instance->receive_messages == false && instance->message_receiver != NULL)
			{
				destroy_message_receiver(instance);
			}

			if (send_pending_events(instance) != RESULT_OK && instance->event_send_retry_limit > 0)
			{
				instance->event_send_error_count++;

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_131: [If messenger_do_work() fail sending events for `instance->event_send_retry_limit` times, it shall invoke `instance->on_state_changed_callback`, if provided, with error code MESSENGER_STATE_ERROR]
				if (instance->event_send_error_count >= instance->event_send_retry_limit)
				{
					LogError("messenger_do_work failed (failed sending events; reached max number of consecutive attempts)");
					update_state(instance, MESSENGER_STATE_ERROR);
				}
			}
			else
			{
				instance->event_send_error_count = 0;
			}
		}
	}
}

void messenger_destroy(MESSENGER_HANDLE messenger_handle)
{
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_109: [If `messenger_handle` is NULL, messenger_destroy() shall fail and return]
	if (messenger_handle == NULL)
	{
		LogError("messenger_destroy failed (messenger_handle is NULL)");
	}
	else
	{
		MESSENGER_INSTANCE* instance = (MESSENGER_INSTANCE*)messenger_handle;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_110: [If the `instance->state` is not MESSENGER_STATE_STOPPED, messenger_destroy() shall invoke messenger_stop(), ignoring its result]
		if (instance->state != MESSENGER_STATE_STOPPED)
		{
			(void)messenger_stop(messenger_handle);
		}

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_111: [All elements of `instance->in_progress_list` shall be moved to the beginning of `instance->wait_to_send_list`]
		DList_InsertHeadList(instance->wait_to_send_list, &instance->in_progress_list);

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_112: [`instance->iothub_host_fqdn` shall be destroyed using STRING_delete()]
		STRING_delete(instance->iothub_host_fqdn);
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_113: [`instance->device_id` shall be destroyed using STRING_delete()]
		STRING_delete(instance->device_id);
		
		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_114: [messenger_destroy() shall destroy `instance` with free()]
		(void)free(instance);
	}
}

MESSENGER_HANDLE messenger_create(const MESSENGER_CONFIG* messenger_config)
{
	MESSENGER_HANDLE handle;

	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_001: [If parameter `messenger_config` is NULL, messenger_create() shall return NULL]
	if (messenger_config == NULL)
	{
		handle = NULL;
		LogError("messenger_create failed (messenger_config is NULL)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_002: [If `messenger_config->device_id` is NULL, messenger_create() shall return NULL]
	else if (messenger_config->device_id == NULL)
	{
		handle = NULL;
		LogError("messenger_create failed (messenger_config->device_id is NULL)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_003: [If `messenger_config->iothub_host_fqdn` is NULL, messenger_create() shall return NULL]
	else if (messenger_config->iothub_host_fqdn == NULL)
	{
		handle = NULL;
		LogError("messenger_create failed (messenger_config->iothub_host_fqdn is NULL)");
	}
	// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_004: [If `messenger_config->wait_to_send_list` is NULL, messenger_create() shall return NULL]
	else if (messenger_config->wait_to_send_list == NULL)
	{
		handle = NULL;
		LogError("messenger_create failed (messenger_config->wait_to_send_list is NULL)");
	}
	else
	{
		MESSENGER_INSTANCE* instance;

		// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_006: [messenger_create() shall allocate memory for the messenger instance structure (aka `instance`)]
		if ((instance = (MESSENGER_INSTANCE*)malloc(sizeof(MESSENGER_INSTANCE))) == NULL)
		{
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_007: [If malloc() fails, messenger_create() shall fail and return NULL]
			handle = NULL;
			LogError("messenger_create failed (messenger_config->wait_to_send_list is NULL)");
		}
		else
		{
			memset(instance, 0, sizeof(MESSENGER_INSTANCE));
			instance->state = MESSENGER_STATE_STOPPED;
			instance->event_send_retry_limit = DEFAULT_EVENT_SEND_RETRY_LIMIT;

			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_008: [messenger_create() shall save a copy of `messenger_config->device_id` into `instance->device_id`]
			if ((instance->device_id = STRING_construct(messenger_config->device_id)) == NULL)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_009: [If STRING_construct() fails, messenger_create() shall fail and return NULL]
				handle = NULL;
				LogError("messenger_create failed (device_id could not be copied; STRING_construct failed)");
			}
			// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_010: [messenger_create() shall save a copy of `messenger_config->iothub_host_fqdn` into `instance->iothub_host_fqdn`]
			else if ((instance->iothub_host_fqdn = STRING_construct(messenger_config->iothub_host_fqdn)) == NULL)
			{
				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_011: [If STRING_construct() fails, messenger_create() shall fail and return NULL]
				handle = NULL;
				LogError("messenger_create failed (iothub_host_fqdn could not be copied; STRING_construct failed)");
			}
			else
			{
				DList_InitializeListHead(&instance->in_progress_list);

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_012: [The pointer `messenger_config->wait_to_send_list` shall be saved into `instance->wait_to_send_list`]
				instance->wait_to_send_list = messenger_config->wait_to_send_list;

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_013: [`messenger_config->on_state_changed_callback` shall be saved into `instance->on_state_changed_callback`]
				instance->on_state_changed_callback = messenger_config->on_state_changed_callback;

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_014: [`messenger_config->on_state_changed_context` shall be saved into `instance->on_state_changed_context`]
				instance->on_state_changed_context = messenger_config->on_state_changed_context;

				// Codes_SRS_IOTHUBTRANSPORT_AMQP_MESSENGER_09_015: [If no failures occurr, messenger_create() shall return a handle to `instance`]
				handle = (MESSENGER_HANDLE)instance;
			}
		}

		if (handle == NULL)
		{
			messenger_destroy((MESSENGER_HANDLE)instance);
		}
	}

	return handle;
}
