// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/agenttime.h" 
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/uniqueid.h"
#include "azure_c_shared_utility/uuid.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_uamqp_c/amqp_definitions_fields.h"
#include "azure_uamqp_c/messaging.h"
#include "internal/iothub_client_private.h"
#include "internal/iothubtransport_amqp_messenger.h"
#include "internal/iothubtransport_amqp_streaming.h"
#include "iothub_client_options.h"

MU_DEFINE_ENUM_STRINGS(AMQP_TYPE, AMQP_TYPE_VALUES);

#define RESULT_OK 0
#define INDEFINITE_TIME ((time_t)(-1))

#define CLIENT_VERSION_PROPERTY_NAME					"com.microsoft:client-version"
#define UNIQUE_ID_BUFFER_SIZE                           37
#define CORRELATION_ID_PROPERTY_NAME                    "com.microsoft:channel-correlation-id"
static const char* CORRELATION_STRING_FORMAT =          "streams:%s";
#define CORRELATION_STRING_FORMAT_BASE_LENGTH           (strlen(CORRELATION_STRING_FORMAT) - 2)
#define REQUEST_ACCEPTED                                "true"
#define REQUEST_REJECTED                                "false"
#define STREAM_API_VERSION_PROPERTY_NAME                "com.microsoft:api-version"
#define STREAM_API_VERSION_NUMBER                       "2016-11-14"

#define EMPTY_AMQP_BODY_DATA                            ((const unsigned char*)" ")
#define EMPTY_AMQP_BODY_SIZE                            1

static const char* DEFAULT_SEND_LINK_SOURCE_NAME =      "streams";
static const char* DEFAULT_RECEIVE_LINK_TARGET_NAME =	"streams";

static const char* STREAM_PROP_NAME =                   "IoThub-streaming-name";
static const char* STREAM_PROP_HOSTNAME =               "IoThub-streaming-hostname";
static const char* STREAM_PROP_PORT =                   "IoThub-streaming-port";
static const char* STREAM_PROP_URL =                    "IoThub-streaming-url";
static const char* STREAM_PROP_AUTH_TOKEN =             "IoThub-streaming-auth-token";
static const char* STREAM_PROP_IS_ACCEPTED =            "IoThub-streaming-is-accepted";

static const char* STREAMING_CLIENT_AMQP_MSGR_OPTIONS = "streaming_client_amqp_msgr_options";


typedef struct AMQP_STREAMING_CLIENT_TAG
{
	pfTransport_GetOption_Product_Info_Callback prod_info_cb;
	void* prod_info_ctx;

	char* device_id;
	char* module_id;
	char* iothub_host_fqdn;

    AMQP_STREAMING_CLIENT_STATE state;

    AMQP_STREAMING_CLIENT_STATE_CHANGED_CALLBACK on_state_changed_callback;
	const void* on_state_changed_context;

    DEVICE_STREAM_C2D_REQUEST_CALLBACK stream_request_callback;
    void* stream_request_context;

	AMQP_MESSENGER_HANDLE amqp_msgr;
	AMQP_MESSENGER_STATE amqp_msgr_state;
	bool amqp_msgr_is_subscribed;
} AMQP_STREAMING_CLIENT;

typedef struct PARSED_STREAM_INFO_TAG
{
    char* request_id;

    char* name;
    char* url;
    char* authorization_token;

    char* hostname;
    size_t port;
} PARSED_STREAM_INFO;


//---------- AMQP Helper Functions ----------//

static int get_correlation_id(PROPERTIES_HANDLE properties, char** correlation_id)
{
    int result;
    AMQP_VALUE amqp_value;

    if (properties_get_correlation_id(properties, &amqp_value) == 0 && amqp_value != NULL)
    {
        AMQP_TYPE amqp_type = amqpvalue_get_type(amqp_value);

        if (amqp_type == AMQP_TYPE_STRING)
        {
            const char* value;

            if (amqpvalue_get_string(amqp_value, &value) != 0)
            {
                LogError("Failed retrieving string from AMQP value");
                result = MU_FAILURE;
            }
            else if (mallocAndStrcpy_s(correlation_id, value) != 0)
            {
                LogError("Failed cloning correlation-id");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        else if (amqp_type == AMQP_TYPE_UUID)
        {
            uuid value;

            if (amqpvalue_get_uuid(amqp_value, &value) != 0)
            {
                LogError("Failed retrieving uuid from AMQP value");
                result = MU_FAILURE;
            }
            else
            {
                char* uuid_string;

                if ((uuid_string = UUID_to_string((const UUID_T*)&value)) == NULL)
                {
                    LogError("Failed converting UUID_T to string");
                    result = MU_FAILURE;
                }
                else
                {
                    *correlation_id = uuid_string;
                    result = RESULT_OK;
                }
            }
        }
        else
        {
            LogError("Unexpected AMQP type (%s)", MU_ENUM_TO_STRING(AMQP_TYPE, amqp_type));
            result = MU_FAILURE;
        }
    }
    else
    {
        *correlation_id = NULL;
        result = RESULT_OK;
    }

    return result;
}

static int parse_message_properties(MESSAGE_HANDLE message, PARSED_STREAM_INFO* parsed_info)
{
	int result;
	PROPERTIES_HANDLE properties = NULL;

	if (message_get_properties(message, &properties) != 0 || properties == NULL)
	{
		LogError("Failed getting the AMQP message properties");
		result = MU_FAILURE;
	}
    else
    {
        if (get_correlation_id(properties, &parsed_info->request_id) != 0)
        {
            LogError("Failed getting the correlation id");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }

        properties_destroy(properties);
    }

	return result;
}

static int set_application_properties(MESSAGE_HANDLE message, const char* correlation_id)
{
    int result;
    PROPERTIES_HANDLE properties = NULL;

    if (message_get_properties(message, &properties) != 0)
    {
        LogError("Failed getting the AMQP message properties");
        result = MU_FAILURE;
    }
    else if (properties == NULL && (properties = properties_create()) == NULL)
    {
        LogError("Failed creating properties for AMQP message");
        result = MU_FAILURE;
    }
    else
    {
        bool encoding_failed = false;

        if (correlation_id != NULL)
        {
            UUID_T uuid_value;

            if (UUID_from_string(correlation_id, &uuid_value) != 0)
            {
                LogError("Failed parsing correation_id into UUID_T");
                encoding_failed = true;
            }
            else
            {
                AMQP_VALUE amqp_value_correlation_id;

                if ((amqp_value_correlation_id = amqpvalue_create_uuid((unsigned char*)uuid_value)) == NULL)
                {
                    LogError("Failed creating AMQP value for correlation-id");
                    encoding_failed = true;
                }
                else
                {
                    if (properties_set_correlation_id(properties, amqp_value_correlation_id) != 0)
                    {
                        LogError("Failed setting the correlation id");
                        encoding_failed = true;
                    }

                    amqpvalue_destroy(amqp_value_correlation_id);
                }
            }
        }

        if (encoding_failed)
        {
            result = MU_FAILURE;
        }
        else
        {
            if (message_set_properties(message, properties) != RESULT_OK)
            {
                LogError("Failed setting the AMQP message properties");
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }

        }

        properties_destroy(properties);
    }

    return result;
}

static int add_application_property(AMQP_VALUE properties, const char* name, AMQP_VALUE value)
{
    int result;
    AMQP_VALUE map_key_value;

    if ((map_key_value = amqpvalue_create_string(name)) == NULL)
    {
        LogError("Failed to create AMQP property key name.");
        result = MU_FAILURE;
    }
    else
    {
        if (amqpvalue_set_map_value(properties, map_key_value, value) != 0)
        {
            LogError("Failed to set key/value into the the AMQP property map.");
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }

        amqpvalue_destroy(map_key_value);
    }

    return result;
}

static int add_bool_application_property(AMQP_VALUE properties, const char* name, bool value)
{
    int result;
    AMQP_VALUE amqp_value;

    if ((amqp_value = amqpvalue_create_boolean(value)) == NULL)
    {
        LogError("Failed to create AMQP value for property value.");
        result = MU_FAILURE;
    }
    else
    {
        result = add_application_property(properties, name, amqp_value);

        amqpvalue_destroy(amqp_value);
    }

    return result;
}

static MESSAGE_HANDLE create_amqp_message_from_stream_c2d_response(DEVICE_STREAM_C2D_RESPONSE* streamResponse)
{
    MESSAGE_HANDLE result;

    if ((result = message_create()) == NULL)
    {
        LogError("Failed creating AMQP message");
    }
    else if (set_application_properties(result, streamResponse->request_id) != RESULT_OK)
    {
        LogError("Failed setting AMQP message properties");
        message_destroy(result);
        result = NULL;
    }
    else
    {
        AMQP_VALUE properties;

        if ((properties = amqpvalue_create_map()) == NULL)
        {
            LogError("Failed to create AMQP map for the properties.");
            message_destroy(result);
            result = NULL;
        }
        else
        {
            if (add_bool_application_property(properties, STREAM_PROP_IS_ACCEPTED, streamResponse->accept) != 0)
            {
                LogError("Failed to set request is-accepted property on the AMQP message.");
                message_destroy(result);
                result = NULL;
            }
            else if (message_set_application_properties(result, properties) != 0)
            {
                LogError("Failed to set the message properties on the AMQP message.");
                message_destroy(result);
                result = NULL;
            }
            else
            {
                BINARY_DATA binary_data;
                binary_data.bytes = EMPTY_AMQP_BODY_DATA;
                binary_data.length = EMPTY_AMQP_BODY_SIZE;

                if (message_add_body_amqp_data(result, binary_data) != 0)
                {
                    LogError("Failed adding AMQP message body");
                    message_destroy(result);
                    result = NULL;
                }
            }

            amqpvalue_destroy(properties);
        }
    }

    return result;
}


static void destroy_parsed_info(PARSED_STREAM_INFO* parsed_info)
{
    if (parsed_info != NULL)
    {
        if (parsed_info->request_id != NULL)
        {
            free(parsed_info->request_id);
            parsed_info->request_id = NULL;
        }

        if (parsed_info->url != NULL)
        {
            free(parsed_info->url);
            parsed_info->url = NULL;
        }

        if (parsed_info->authorization_token != NULL)
        {
            free(parsed_info->authorization_token);
            parsed_info->authorization_token = NULL;
        }

        if (parsed_info->name != NULL)
        {
            free(parsed_info->name);
            parsed_info->name = NULL;
        }

        if (parsed_info->hostname != NULL)
        {
            free(parsed_info->hostname);
            parsed_info->hostname = NULL;
        }

        // Do not destroy "parsed_info" on purpose.
    }
}

static int parse_message_application_properties(MESSAGE_HANDLE message, PARSED_STREAM_INFO* parsed_info)
{
    int result;
    AMQP_VALUE application_properties = NULL;

    if (message_get_application_properties(message, &application_properties) != 0)
    {
        LogError("Failed getting the AMQP message application properties");
        result = MU_FAILURE;
    }
    else
    {
        if (application_properties == NULL)
        {
            result = MU_FAILURE;
        }
        else
        {
            AMQP_VALUE application_properties_ipdv;
            uint32_t property_count = 0;

            if ((application_properties_ipdv = amqpvalue_get_inplace_described_value(application_properties)) == NULL)
            {
                LogError("Failed getting the map of AMQP message application properties.");
                result = MU_FAILURE;
            }
            else if ((result = amqpvalue_get_map_pair_count(application_properties_ipdv, &property_count)) != 0)
            {
                LogError("Failed reading the number of values in the AMQP property map.");
                result = MU_FAILURE;
            }
            else
            {
                uint32_t i;

                result = RESULT_OK;

                for (i = 0; result == RESULT_OK && i < property_count; i++)
                {
                    AMQP_VALUE map_key_name = NULL;
                    AMQP_VALUE map_key_value = NULL;
                    const char *key_name = NULL;
                    const char* key_value = NULL;

                    if (amqpvalue_get_map_key_value_pair(application_properties_ipdv, i, &map_key_name, &map_key_value) != 0)
                    {
                        LogError("Failed reading the key/value pair from the uAMQP property map.");
                        result = MU_FAILURE;
                    }
                    else if (amqpvalue_get_string(map_key_name, &key_name) != 0)
                    {
                        LogError("Failed parsing the uAMQP property name");
                        result = MU_FAILURE;
                    }
                    else if (amqpvalue_get_string(map_key_value, &key_value) != 0)
                    {
                        LogError("Failed parsing the uAMQP property value");
                        result = MU_FAILURE;
                    }
                    else if (key_name == NULL || key_value == NULL)
                    {
                        LogError("Invalid property name (%p) and/or value (%p)", key_name, key_value);
                        result = MU_FAILURE;
                    }
                    else if (strcmp(STREAM_PROP_NAME, key_name) == 0)
                    {
                        if (mallocAndStrcpy_s(&parsed_info->name, key_value) != 0)
                        {
                            LogError("Failed copying property '%s'", key_name);
                            result = MU_FAILURE;
                        }
                    }
                    else if (strcmp(STREAM_PROP_URL, key_name) == 0)
                    {
                        if (mallocAndStrcpy_s(&parsed_info->url, key_value) != 0)
                        {
                            LogError("Failed copying property '%s'", key_name);
                            result = MU_FAILURE;
                        }
                    }
                    else if (strcmp(STREAM_PROP_AUTH_TOKEN, key_name) == 0)
                    {
                        if (mallocAndStrcpy_s(&parsed_info->authorization_token, key_value) != 0)
                        {
                            LogError("Failed copying property '%s'", key_name);
                            result = MU_FAILURE;
                        }
                    }
                    else if (strcmp(STREAM_PROP_HOSTNAME, key_name) == 0)
                    {
                        if (mallocAndStrcpy_s(&parsed_info->hostname, key_value) != 0)
                        {
                            LogError("Failed copying property '%s'", key_name);
                            result = MU_FAILURE;
                        }
                    }
                    else if (strcmp(STREAM_PROP_PORT, key_name) == 0)
                    {
                        parsed_info->port = (size_t)atoi(key_value);
                    }


                    if (map_key_name != NULL)
                    {
                        amqpvalue_destroy(map_key_name);
                    }

                    if (map_key_value != NULL)
                    {
                        amqpvalue_destroy(map_key_value);
                    }
                }
            }

            amqpvalue_destroy(application_properties);
        }
    }

    return result;
}

static int parse_amqp_message(MESSAGE_HANDLE message, PARSED_STREAM_INFO* parsed_info)
{
    int result;

    (void)memset(parsed_info, 0, sizeof(PARSED_STREAM_INFO));

    if (parse_message_properties(message, parsed_info) != 0)
    {
        LogError("Failed parsing message properties.");
        destroy_parsed_info(parsed_info);
        result = MU_FAILURE;
    }
    else if (parse_message_application_properties(message, parsed_info) != 0)
    {
        // TODO: if application properties are empty, body may contain error message. Gotta process that.
        LogError("Failed parsing message application properties.");
        destroy_parsed_info(parsed_info);
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}


//---------- Generic Helpers ----------//

static char* generate_unique_id()
{
	char* result;

	if ((result = (char*)malloc(sizeof(char) * UNIQUE_ID_BUFFER_SIZE + 1)) == NULL)
	{
		LogError("Failed generating an unique tag (malloc failed)");
	}
	else
	{
		memset(result, 0, sizeof(char) * UNIQUE_ID_BUFFER_SIZE + 1);

		if (UniqueId_Generate(result, UNIQUE_ID_BUFFER_SIZE) != UNIQUEID_OK)
		{
			LogError("Failed generating an unique tag (UniqueId_Generate failed)");
			free(result);
			result = NULL;
		}
	}

	return result;
}

static char* generate_correlation_id_string()
{
    char* result;
    char* unique_id;

    if ((unique_id = generate_unique_id()) == NULL)
    {
        LogError("Failed generating unique ID for correlation-id");
        result = NULL;
    }
    else
    {
        if ((result = (char*)malloc(CORRELATION_STRING_FORMAT_BASE_LENGTH + strlen(unique_id) + 1)) == NULL)
        {
            LogError("Failed allocating correlation-id string");
        }
        else if (sprintf(result, CORRELATION_STRING_FORMAT, unique_id) <= 0)
        {
            LogError("Failed defining correlation-id string");
            free(result);
            result = NULL;
        }

        free(unique_id);
    }

    return result;
}

static void update_state(AMQP_STREAMING_CLIENT* streaming_client, AMQP_STREAMING_CLIENT_STATE new_state)
{
    if (new_state != streaming_client->state)
    {
        AMQP_STREAMING_CLIENT_STATE previous_state = streaming_client->state;
        streaming_client->state = new_state;

        if (streaming_client->on_state_changed_callback != NULL)
        {
            streaming_client->on_state_changed_callback(streaming_client->on_state_changed_context, previous_state, new_state);
        }
    }
}

static int internal_amqp_streaming_client_stop(AMQP_STREAMING_CLIENT* streaming_client)
{
    int result;

    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_025: [`instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_STOPPING, and `instance->on_state_changed_callback` invoked if provided]  
    update_state(streaming_client, AMQP_STREAMING_CLIENT_STATE_STOPPING);

    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_023: [amqp_messenger_stop() shall be invoked passing `instance->amqp_msgr`]  
    if (amqp_messenger_stop(streaming_client->amqp_msgr) != 0)
    {
        LogError("Failed stopping the AMQP messenger (%s)", streaming_client->device_id);
        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_026: [If any failures occur, `instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided]
        update_state(streaming_client, AMQP_STREAMING_CLIENT_STATE_ERROR);
        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_024: [If amqp_messenger_stop() fails, amqp_streaming_client_stop() fail and return a non-zero value]
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_027: [If no failures occur, amqp_streaming_client_stop() shall return 0]
        result = RESULT_OK;
    }

    return result;
}

static void internal_streaming_client_destroy(AMQP_STREAMING_CLIENT* streaming_client)
{
    if (streaming_client->amqp_msgr != NULL)
    {
        // Codes_IOTHUBTRANSPORT_AMQP_AMQP_STREAMING_CLIENT_09_099: [`streaming_client->amqp_messenger` shall be destroyed using amqp_messenger_destroy()]  
        amqp_messenger_destroy(streaming_client->amqp_msgr);
    }

    if (streaming_client->device_id != NULL)
    {
        free(streaming_client->device_id);
    }

    if (streaming_client->module_id != NULL)
    {
        free(streaming_client->module_id);
    }

    if (streaming_client->iothub_host_fqdn != NULL)
    {
        free(streaming_client->iothub_host_fqdn);
    }

    free(streaming_client);
}

static void destroy_link_attach_properties(MAP_HANDLE properties)
{
    Map_Destroy(properties);
}

static MAP_HANDLE create_link_attach_properties(AMQP_STREAMING_CLIENT* streaming_client)
{
    MAP_HANDLE result;
    char* correlation_string;

    if ((correlation_string = generate_correlation_id_string()) == NULL)
    {
        LogError("Failed creating AMQP link property 'correlation-id' (%s)", streaming_client->device_id);
        result = NULL;
    }
    else
    {
        if ((result = Map_Create(NULL)) == NULL)
        {
            LogError("Failed creating map for AMQP link properties (%s)", streaming_client->device_id);
        }
        else if (Map_Add(result, CORRELATION_ID_PROPERTY_NAME, correlation_string) != MAP_OK)
        {
            LogError("Failed adding AMQP link property 'correlation-id' (%s)", streaming_client->device_id);
            destroy_link_attach_properties(result);
            result = NULL;
        }
        else if (Map_Add(result, CLIENT_VERSION_PROPERTY_NAME, streaming_client->prod_info_cb(streaming_client->prod_info_ctx)) != MAP_OK)
        {
            LogError("Failed adding AMQP link property 'prod_info_cb' (%s)", streaming_client->device_id);
            destroy_link_attach_properties(result);
            result = NULL;
        }
        else if (Map_Add(result, STREAM_API_VERSION_PROPERTY_NAME, STREAM_API_VERSION_NUMBER) != MAP_OK)
        {
            LogError("Failed adding AMQP link property 'api-version' (%s)", streaming_client->device_id);
            destroy_link_attach_properties(result);
            result = NULL;
        }

        free(correlation_string);
    }

    return result;
}

static DEVICE_STREAM_C2D_REQUEST* create_stream_c2d_request_from_parsed_info(PARSED_STREAM_INFO* parsed_info)
{
    DEVICE_STREAM_C2D_REQUEST* result;

    if ((result = (DEVICE_STREAM_C2D_REQUEST*)malloc(sizeof(DEVICE_STREAM_C2D_REQUEST))) == NULL)
    {
        LogError("Failed creating a new instance of DEVICE_STREAM_C2D_REQUEST");
    }
    else
    {
        memset(result, 0, sizeof(DEVICE_STREAM_C2D_REQUEST));

        if (mallocAndStrcpy_s(&result->request_id, parsed_info->request_id) != 0)
        {
            LogError("Failed setting stream c2d response request-id");
            stream_c2d_request_destroy(result);
            result = NULL;
        }
        else if (mallocAndStrcpy_s(&result->name, parsed_info->name) != 0)
        {
            LogError("Failed setting stream c2d response name");
            stream_c2d_request_destroy(result);
            result = NULL;
        }
        else if (mallocAndStrcpy_s(&result->url, parsed_info->url) != 0)
        {
            LogError("Failed setting stream c2d response url");
            stream_c2d_request_destroy(result);
            result = NULL;
        }
        else if (mallocAndStrcpy_s(&result->authorization_token, parsed_info->authorization_token) != 0)
        {
            LogError("Failed setting stream c2d response authorization token");
            stream_c2d_request_destroy(result);
            result = NULL;
        }
    }

    return result;
}


//---------- Callbacks ----------//

static void on_amqp_messenger_state_changed_callback(void* context, AMQP_MESSENGER_STATE previous_state, AMQP_MESSENGER_STATE new_state)
{
    if (context == NULL)
    {
        LogError("Invalid argument (context is NULL)");
    }
    else if (new_state != previous_state)
    {
        AMQP_STREAMING_CLIENT* streaming_client = (AMQP_STREAMING_CLIENT*)context;

        if (streaming_client->state == AMQP_STREAMING_CLIENT_STATE_STARTING && new_state == AMQP_MESSENGER_STATE_STARTED)
        {
            if (streaming_client->amqp_msgr_is_subscribed)
            {
                update_state(streaming_client, AMQP_STREAMING_CLIENT_STATE_STARTED);
            }
            // Else, it shall wait for the moment the AMQP msgr is subscribed.
        }
        else if (streaming_client->state == AMQP_STREAMING_CLIENT_STATE_STOPPING && new_state == AMQP_STREAMING_CLIENT_STATE_STOPPED)
        {
            if (!streaming_client->amqp_msgr_is_subscribed)
            {
                update_state(streaming_client, AMQP_STREAMING_CLIENT_STATE_STOPPED);
            }
            // Else, it shall wait for the moment the AMQP msgr is unsubscribed.
        }
        else if ((streaming_client->state == AMQP_STREAMING_CLIENT_STATE_STARTING && new_state == AMQP_MESSENGER_STATE_STARTING) ||
            (streaming_client->state == AMQP_STREAMING_CLIENT_STATE_STOPPING && new_state == AMQP_MESSENGER_STATE_STOPPING))
        {
            // Do nothing, this is expected.
        }
        else
        {
            LogError("Unexpected AMQP messenger state (%s, %s, %s)",
                streaming_client->device_id, MU_ENUM_TO_STRING(AMQP_STREAMING_CLIENT_STATE, streaming_client->state), MU_ENUM_TO_STRING(AMQP_MESSENGER_STATE, new_state));

            update_state(streaming_client, AMQP_STREAMING_CLIENT_STATE_ERROR);
        }

        streaming_client->amqp_msgr_state = new_state;
    }
}

static void on_amqp_messenger_subscription_changed_callback(void* context, bool is_subscribed)
{
    if (context == NULL)
    {
        LogError("Invalid argument (context is NULL)");
    }
    else
    {
        AMQP_STREAMING_CLIENT* streaming_client = (AMQP_STREAMING_CLIENT*)context;

        if (streaming_client->state == AMQP_STREAMING_CLIENT_STATE_STARTING && is_subscribed)
        {
            if (streaming_client->amqp_msgr_state == AMQP_MESSENGER_STATE_STARTED)
            {
                update_state(streaming_client, AMQP_STREAMING_CLIENT_STATE_STARTED);
            }
            // Else, it shall wait for the moment the AMQP msgr is STARTED.
        }
        else if (streaming_client->state == AMQP_STREAMING_CLIENT_STATE_STOPPING && !is_subscribed)
        {
            if (streaming_client->amqp_msgr_state == AMQP_MESSENGER_STATE_STOPPED)
            {
                update_state(streaming_client, AMQP_STREAMING_CLIENT_STATE_STOPPED);
            }
            // Else, it shall wait for the moment the AMQP msgr is STOPPED.
        }
        else
        {
            LogError("Unexpected AMQP messenger state (%s, %s, %d)",
                streaming_client->device_id, MU_ENUM_TO_STRING(AMQP_STREAMING_CLIENT_STATE, streaming_client->state), is_subscribed);

            update_state(streaming_client, AMQP_STREAMING_CLIENT_STATE_ERROR);
        }

        streaming_client->amqp_msgr_is_subscribed = is_subscribed;
    }
}

static void on_amqp_send_response_complete_callback(AMQP_MESSENGER_SEND_RESULT result, AMQP_MESSENGER_REASON reason, void* context)
{
    if (context == NULL)
    {
        LogError("Invalid argument (context is NULL)");
    }
    else
    {
        AMQP_STREAMING_CLIENT* client = (AMQP_STREAMING_CLIENT*)context;

        if (result != AMQP_MESSENGER_SEND_RESULT_SUCCESS)
        {
            LogError("Failed sending AMQP message (%s, %s, %s)",
                client->module_id != NULL ? client->module_id : client->device_id,
                MU_ENUM_TO_STRING(AMQP_MESSENGER_SEND_RESULT, result), MU_ENUM_TO_STRING(AMQP_MESSENGER_REASON, reason));

            update_state(client, AMQP_STREAMING_CLIENT_STATE_ERROR);
        }
    }
}

static int send_stream_c2d_response(AMQP_STREAMING_CLIENT* client, DEVICE_STREAM_C2D_RESPONSE* response)
{
    int result;
    MESSAGE_HANDLE amqp_message;

    if ((amqp_message = create_amqp_message_from_stream_c2d_response(response)) == NULL)
    {
        LogError("Failed creating AMQP message for stream response");
        result = MU_FAILURE;
    }
    else
    {
        if (amqp_messenger_send_async(client->amqp_msgr, amqp_message, on_amqp_send_response_complete_callback, client) != 0)
        {
            LogError("Failed sending response message for (%s, %s)", client->module_id != NULL ? client->module_id : client->device_id, response->request_id);
            result = MU_FAILURE;
        }
        else
        {
            result = RESULT_OK;
        }

        message_destroy(amqp_message);
    }

    return result;
}

static AMQP_MESSENGER_DISPOSITION_RESULT on_amqp_message_received_callback(MESSAGE_HANDLE message, AMQP_MESSENGER_MESSAGE_DISPOSITION_INFO* disposition_info, void* context)
{
    AMQP_MESSENGER_DISPOSITION_RESULT disposition_result;
    
    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_048: [If `message` or `context` are NULL, on_amqp_message_received_callback shall return immediately]
    if (message == NULL || context == NULL)
    {
        LogError("Invalid argument (message=%p, context=%p)", message, context);
        disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED;
    }
    else
    {
        AMQP_STREAMING_CLIENT* streaming_client = (AMQP_STREAMING_CLIENT*)context;
        PARSED_STREAM_INFO parsed_info;

        amqp_messenger_destroy_disposition_info(disposition_info);

        if (parse_amqp_message(message, &parsed_info) != 0)
        {
            LogError("Failed parsing AMQP message");
            disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED;
        }
        else
        {
            if (parsed_info.request_id == NULL)
            {
                LogError("Invalid stream request info parsed from AMQP message");
                disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED;
            }
            else
            {
                DEVICE_STREAM_C2D_REQUEST* request;

                if ((request = create_stream_c2d_request_from_parsed_info(&parsed_info)) == NULL)
                {
                    LogError("Failed creating stream c2d request from parsed amqp message");
                    disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED;
                }
                else
                {
                    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_049: [ `streamRequestCallback` shall be invoked passing the parsed STREAM_REQUEST_INFO instance ]  
                    DEVICE_STREAM_C2D_RESPONSE* response = streaming_client->stream_request_callback(request, streaming_client->stream_request_context);

                    if (response != NULL)
                    {
                        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_050: [ The response from `streamRequestCallback` shall be saved to be sent on the next call to _do_work() ]
                        if (send_stream_c2d_response(streaming_client, response) != 0)
                        {
                            LogError("Failed sending stream c2d response");
                            disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_REJECTED;
                        }
                        else
                        {
                            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_052: [ If no parsing errors occur, `on_amqp_message_received_callback` shall return AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED ]
                            disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED;
                        }

                        stream_c2d_response_destroy(response);
                    }
                    else
                    {
                        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_052: [ If no parsing errors occur, `on_amqp_message_received_callback` shall return AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED ]
                        disposition_result = AMQP_MESSENGER_DISPOSITION_RESULT_ACCEPTED;
                    }

                    stream_c2d_request_destroy(request);
                }
            }

            destroy_parsed_info(&parsed_info);
        }
    }

    return disposition_result;
}


// ---------- Set/Retrieve Options Helpers ----------//

static void* clone_option(const char* name, const void* value)
{
    void* result;

    if (name == NULL || value == NULL)
    {
        LogError("Invalid argument (name=%p, value=%p)", name, value);
        result = NULL;
    }
    else if (strcmp(STREAMING_CLIENT_AMQP_MSGR_OPTIONS, name) == 0)
    {
        if ((result = (void*)OptionHandler_Clone((OPTIONHANDLER_HANDLE)value)) == NULL)
        {
            LogError("Failed cloning option '%s'", name);
        }
    }
    else
    {
        LogError("Failed to clone messenger option (option with name '%s' is not suppported)", name);
        result = NULL;
    }

    return result;
}

static void destroy_option(const char* name, const void* value)
{
    if (name == NULL || value == NULL)
    {
        LogError("Invalid argument (name=%p, value=%p)", name, value);
    }
    else if (strcmp(STREAMING_CLIENT_AMQP_MSGR_OPTIONS, name) == 0)
    {
        OptionHandler_Destroy((OPTIONHANDLER_HANDLE)value);
    }
    else
    {
        LogError("Invalid argument (option '%s' is not suppported)", name);
    }
}



//---------- Public API ----------//

AMQP_STREAMING_CLIENT_HANDLE amqp_streaming_client_create(const AMQP_STREAMING_CLIENT_CONFIG* client_config)
{
    AMQP_STREAMING_CLIENT* result;
 
    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_001: [If parameter `client_config` is NULL, amqp_streaming_client_create() shall return NULL]  
    if (client_config == NULL)
    {
        LogError("Invalid argument (client_config is NULL)");
        result = NULL;
    }
    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_002: [If `client_config`'s `device_id`, `iothub_host_fqdn` or `client_version` is NULL, amqp_streaming_client_create() shall return NULL]  
    else if (client_config->device_id == NULL || client_config->iothub_host_fqdn == NULL || client_config->prod_info_cb == NULL)
    {
        LogError("Invalid argument (device_id=%p, iothub_host_fqdn=%p, prod_info_cb=%p)",
            client_config->device_id, client_config->iothub_host_fqdn, client_config->prod_info_cb);
        result = NULL;
    }
    else
    {
        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_003: [amqp_streaming_client_create() shall allocate memory for the messenger instance structure (aka `instance`)]  
        if ((result = (AMQP_STREAMING_CLIENT*)malloc(sizeof(AMQP_STREAMING_CLIENT))) == NULL)
        {
            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_004: [If malloc() fails, amqp_streaming_client_create() shall fail and return NULL]
            LogError("Failed allocating AMQP_STREAMING_CLIENT (%s)", client_config->device_id);
        }
        else
        {
            MAP_HANDLE link_attach_properties;

            memset(result, 0, sizeof(AMQP_STREAMING_CLIENT));
            result->state = AMQP_STREAMING_CLIENT_STATE_STOPPED;
            result->amqp_msgr_state = AMQP_MESSENGER_STATE_STOPPED;

            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_005: [amqp_streaming_client_create() shall save a copy of `client_config` info into `instance`]
            result->prod_info_cb = client_config->prod_info_cb;
            result->prod_info_ctx = client_config->prod_info_ctx;
            
            if (mallocAndStrcpy_s(&result->device_id, client_config->device_id) != 0)
            {
                // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_006: [If any `client_config` info fails to be copied, amqp_streaming_client_create() shall fail and return NULL]
                LogError("Failed copying device_id (%s)", client_config->device_id);
                internal_streaming_client_destroy(result);
                result = NULL;
            }
            else if ((client_config->module_id != NULL) && (mallocAndStrcpy_s(&result->module_id, client_config->module_id) != 0))
            {
                // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_006: [If any `client_config` info fails to be copied, amqp_streaming_client_create() shall fail and return NULL]
                LogError("Failed copying module_id (%s)", client_config->device_id);
                internal_streaming_client_destroy(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->iothub_host_fqdn, client_config->iothub_host_fqdn) != 0)
            {
                // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_006: [If any `client_config` info fails to be copied, amqp_streaming_client_create() shall fail and return NULL]
                LogError("Failed copying iothub_host_fqdn (%s)", client_config->device_id);
                internal_streaming_client_destroy(result);
                result = NULL;
            }
            else if ((link_attach_properties = create_link_attach_properties(result)) == NULL)
            {
                LogError("Failed creating link attach properties (%s)", client_config->device_id);
                internal_streaming_client_destroy(result);
                result = NULL;
            }
            else
            {
                // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_008: [`amqp_msgr_config->client_version` shall be set with `instance->client_version`]
                // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_009: [`amqp_msgr_config->device_id` shall be set with `instance->device_id`]
                // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_010: [`amqp_msgr_config->iothub_host_fqdn` shall be set with `instance->iothub_host_fqdn`]
                // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_011: [`amqp_msgr_config` shall have "streams/" as send link target suffix and receive link source suffix]
                // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_012: [`amqp_msgr_config` shall have send and receive link attach properties set as "com.microsoft:client-version" = `instance->client_version`, "com.microsoft:channel-correlation-id" = `stream:<UUID>`, "com.microsoft:api-version" = "2016-11-14"]
                // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_013: [`amqp_msgr_config` shall be set with `on_amqp_messenger_state_changed_callback` and `on_amqp_messenger_subscription_changed_callback` callbacks]
                AMQP_MESSENGER_CONFIG amqp_msgr_config;
                amqp_msgr_config.prod_info_cb = result->prod_info_cb;
                amqp_msgr_config.prod_info_ctx = result->prod_info_ctx;
                amqp_msgr_config.device_id = result->device_id;
                amqp_msgr_config.module_id = result->module_id;
                amqp_msgr_config.iothub_host_fqdn = result->iothub_host_fqdn;
                amqp_msgr_config.send_link.target_suffix = (char*)DEFAULT_SEND_LINK_SOURCE_NAME;
                amqp_msgr_config.send_link.attach_properties = link_attach_properties;
                amqp_msgr_config.send_link.snd_settle_mode = sender_settle_mode_settled;
                amqp_msgr_config.send_link.rcv_settle_mode = receiver_settle_mode_first;
                amqp_msgr_config.receive_link.source_suffix = (char*)DEFAULT_RECEIVE_LINK_TARGET_NAME;
                amqp_msgr_config.receive_link.attach_properties = link_attach_properties;
                amqp_msgr_config.receive_link.snd_settle_mode = sender_settle_mode_settled;
                amqp_msgr_config.receive_link.rcv_settle_mode = receiver_settle_mode_first;
                amqp_msgr_config.on_state_changed_callback = on_amqp_messenger_state_changed_callback;
                amqp_msgr_config.on_state_changed_context = (void*)result;
                amqp_msgr_config.on_subscription_changed_callback = on_amqp_messenger_subscription_changed_callback;
                amqp_msgr_config.on_subscription_changed_context = (void*)result;

                // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_007: [`instance->amqp_msgr` shall be set using amqp_messenger_create(), passing a AMQP_MESSENGER_CONFIG instance `amqp_msgr_config`]
                if ((result->amqp_msgr = amqp_messenger_create(&amqp_msgr_config)) == NULL)
                {
                    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_014: [If amqp_messenger_create() fails, amqp_streaming_client_create() shall fail and return NULL]
                    LogError("Failed creating the AMQP messenger (%s)", client_config->device_id);
                    internal_streaming_client_destroy(result);
                    result = NULL;
                }
                else if (amqp_messenger_subscribe_for_messages(result->amqp_msgr, on_amqp_message_received_callback, (void*)result))
                {
                    LogError("Failed subscribing for AMQP messages (%s)", client_config->device_id);
                    internal_streaming_client_destroy(result);
                    result = NULL;
                }
                else
                {
                    result->on_state_changed_callback = client_config->on_state_changed_callback;
                    result->on_state_changed_context = client_config->on_state_changed_context;
                }

                destroy_link_attach_properties(link_attach_properties);
            }
        }
    }

    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_015: [If no failures occur, amqp_streaming_client_create() shall return a handle to `instance`]
    return result;
}

int amqp_streaming_client_start(AMQP_STREAMING_CLIENT_HANDLE client_handle, SESSION_HANDLE session_handle)
{
    int result;

    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_016: [If `instance_handle` or `session_handle` are NULL, amqp_streaming_client_start() shall fail and return a non-zero value]
    if (client_handle == NULL || session_handle == NULL)
    {
        LogError("Invalid argument (client_handle=%p, session_handle=%p)", client_handle, session_handle);
        result = MU_FAILURE;
    }
    else
    {
        AMQP_STREAMING_CLIENT* streaming_client = (AMQP_STREAMING_CLIENT*)client_handle;

        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_019: [If no failures occur, `instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_STARTING, and `instance->on_state_changed_callback` invoked if provided]
        update_state(streaming_client, AMQP_STREAMING_CLIENT_STATE_STARTING);

        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_017: [amqp_messenger_start() shall be invoked passing `instance->amqp_msgr` and `session_handle`]  
        if (amqp_messenger_start(streaming_client->amqp_msgr, session_handle) != 0)
        {
            LogError("Failed starting the AMQP messenger (%s)", streaming_client->device_id);
            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_020: [If any failures occur, `instance->state` shall be set to AMQP_STREAMING_CLIENT_STATE_ERROR, and `instance->on_state_changed_callback` invoked if provided]
            update_state(streaming_client, AMQP_STREAMING_CLIENT_STATE_ERROR);
            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_018: [If amqp_messenger_start() fails, amqp_streaming_client_start() fail and return a non-zero value]
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_021: [If no failures occur, amqp_streaming_client_start() shall return 0]
            result = RESULT_OK;
        }
    }

    return result;
}

int amqp_streaming_client_stop(AMQP_STREAMING_CLIENT_HANDLE client_handle)
{
    int result;

    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_022: [If `instance_handle` is NULL, amqp_streaming_client_stop() shall fail and return a non-zero value]  
    if (client_handle == NULL)
    {
        LogError("Invalid argument (client_handle is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        result = internal_amqp_streaming_client_stop((AMQP_STREAMING_CLIENT*)client_handle);
    }

    return result;
}

void amqp_streaming_client_do_work(AMQP_STREAMING_CLIENT_HANDLE client_handle)
{
    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_028: [If `instance_handle` is NULL, amqp_streaming_client_do_work() shall return immediately]
    if (client_handle != NULL)
    {
        AMQP_STREAMING_CLIENT* streaming_client = (AMQP_STREAMING_CLIENT*)client_handle;

        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_031: [amqp_streaming_client_do_work() shall invoke amqp_messenger_do_work() passing `instance->amqp_msgr`]
        amqp_messenger_do_work(streaming_client->amqp_msgr);
    }
}

void amqp_streaming_client_destroy(AMQP_STREAMING_CLIENT_HANDLE client_handle)
{
    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_032: [If `instance_handle` is NULL, amqp_streaming_client_destroy() shall return immediately]  
    if (client_handle == NULL)
    {
        LogError("Invalid argument (client_handle is NULL)");
    }
    else
    {
        AMQP_STREAMING_CLIENT* streaming_client = (AMQP_STREAMING_CLIENT*)client_handle;

        if (streaming_client->state != AMQP_STREAMING_CLIENT_STATE_STOPPING && 
            streaming_client->state != AMQP_STREAMING_CLIENT_STATE_STOPPED)
        {
            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_033: [`instance->amqp_messenger` shall be destroyed using amqp_messenger_destroy()]  
            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_034: [amqp_streaming_client_destroy() shall release all memory allocated for and within `instance`]  
            (void)internal_amqp_streaming_client_stop(streaming_client);
        }

        internal_streaming_client_destroy(streaming_client);
    }
}

int amqp_streaming_client_set_option(AMQP_STREAMING_CLIENT_HANDLE client_handle, const char* name, void* value)
{
    int result;

    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_035: [If `instance_handle` or `name` or `value` are NULL, amqp_streaming_client_set_option() shall fail and return a non-zero value]
    if (client_handle == NULL || name == NULL || value == NULL)
    {
        LogError("Invalid argument (client_handle=%p, name=%p, value=%p)", client_handle, name, value);
        result = MU_FAILURE;
    }
    else
    {
        AMQP_STREAMING_CLIENT* streaming_client = (AMQP_STREAMING_CLIENT*)client_handle;

        if (strcmp(STREAMING_CLIENT_AMQP_MSGR_OPTIONS, name) == 0)
        {
            if (OptionHandler_FeedOptions((OPTIONHANDLER_HANDLE)value, streaming_client->amqp_msgr) != OPTIONHANDLER_OK)
            {
                LogError("Failed feeding options to AMQP messenger (%s)", streaming_client->device_id);
                result = MU_FAILURE;
            }
            else
            {
                result = RESULT_OK;
            }
        }
        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_036: [amqp_messenger_set_option() shall be invoked passing `name` and `option`]
        else if (amqp_messenger_set_option(streaming_client->amqp_msgr, name, value) != RESULT_OK)
        {
            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_037: [If amqp_messenger_set_option() fails, amqp_streaming_client_set_option() shall fail and return a non-zero value]
            LogError("Failed setting TWIN messenger option (%s, %s)", streaming_client->device_id, name);
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_038: [If no errors occur, amqp_streaming_client_set_option shall return zero]
            result = RESULT_OK;
        }
    }

    return result;
}

OPTIONHANDLER_HANDLE amqp_streaming_client_retrieve_options(AMQP_STREAMING_CLIENT_HANDLE client_handle)
{
    OPTIONHANDLER_HANDLE result;

    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_039: [If `instance_handle` is NULL, amqp_streaming_client_retrieve_options shall fail and return NULL]
    if (client_handle == NULL)
    {
        LogError("Invalid argument (client_handle is NULL)");
        result = NULL;
    }
    else
    {
        AMQP_STREAMING_CLIENT* streaming_client = (AMQP_STREAMING_CLIENT*)client_handle;

        if ((result = OptionHandler_Create(clone_option, destroy_option, (pfSetOption)amqp_streaming_client_set_option)) == NULL)
        {
            LogError("Failed creating an OptionHandler (%s)", streaming_client->device_id);
        }
        else
        {
            OPTIONHANDLER_HANDLE amqp_msgr_options;

            // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_040: [amqp_streaming_client_retrieve_options() shall return the result of amqp_messenger_retrieve_options()]
            if ((amqp_msgr_options = amqp_messenger_retrieve_options(streaming_client->amqp_msgr)) == NULL)
            {
                LogError("Failed retrieving amqp messenger options (%s)", streaming_client->device_id);
                OptionHandler_Destroy(result);
                result = NULL;
            }
            else if (OptionHandler_AddOption(result, STREAMING_CLIENT_AMQP_MSGR_OPTIONS, amqp_msgr_options) != OPTIONHANDLER_OK)
            {
                LogError("Failed saving amqp messenger options (%s)", streaming_client->device_id);
                OptionHandler_Destroy(result);
                result = NULL;
            }
        }
    }

    return result;
}

int amqp_streaming_client_set_stream_request_callback(AMQP_STREAMING_CLIENT_HANDLE client, DEVICE_STREAM_C2D_REQUEST_CALLBACK streamRequestCallback, void* context)
{
    int result;

    // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_041: [ If `instance_handle` is NULL, the function shall return a non-zero value (failure) ]
    if (client == NULL)
    {
        LogError("Invalid argument (client is NULL)");
        result = MU_FAILURE;
    }
    else
    {
        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_042: [ `streamRequestCallback` and `context` shall be saved in `instance_handle` to be used in `on_amqp_message_received_callback` ]
        client->stream_request_callback = streamRequestCallback;
        client->stream_request_context = context;

        // Codes_SRS_IOTHUBTRANSPORT_AMQP_STREAMING_09_047: [ If no errors occur, the function shall return zero (success) ]
        result = RESULT_OK;
    }

    return result;
}

int amqp_streaming_client_send_stream_response(AMQP_STREAMING_CLIENT_HANDLE client, DEVICE_STREAM_C2D_RESPONSE* streamResponse)
{
    int result;

    if (client == NULL || streamResponse == NULL)
    {
        LogError("Invalid argument (client=%p, streamResponse=%p)", client, streamResponse);
        result = MU_FAILURE;
    }
    else if (client->amqp_msgr_state != AMQP_MESSENGER_STATE_STARTED)
    {
        LogError("The streaming client is not ready");
        result = MU_FAILURE;
    }
    else if (streamResponse->request_id == NULL)
    {
        LogError("Response does not contain a request id");
        result = MU_FAILURE;
    }
    else if (send_stream_c2d_response(client, streamResponse) != 0)
    {
        LogError("Failed sending stream c2d response");
        result = MU_FAILURE;
    }
    else
    {
        result = RESULT_OK;
    }

    return result;
}
