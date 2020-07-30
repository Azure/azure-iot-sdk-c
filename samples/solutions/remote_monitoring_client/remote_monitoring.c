// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample code works with the Remote Monitoring solution accelerator and shows how to:
// - Report device capabilities.
// - Send telemetry.
// - Respond to methods, including a long-running firmware update method.
// The code simulates a simple Chiller device.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values are omitted for brevity.  Please practice sound engineering practices 
// when writing production code.

#include <stdio.h>
#include <stdlib.h>

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothubtransportmqtt.h"
#include "parson.h"

#define MESSAGERESPONSE(code, message) const char deviceMethodResponse[] = message; \
	*response_size = sizeof(deviceMethodResponse) - 1;                              \
	*response = malloc(*response_size);                                             \
	(void)memcpy(*response, deviceMethodResponse, *response_size);                  \
	result = code;                                                                  \

#define FIRMWARE_UPDATE_STATUS_VALUES \
    DOWNLOADING,                      \
    APPLYING,                         \
    REBOOTING,                        \
    IDLE                              \

/*Enumeration specifying firmware update status */
MU_DEFINE_ENUM(FIRMWARE_UPDATE_STATUS, FIRMWARE_UPDATE_STATUS_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(FIRMWARE_UPDATE_STATUS, FIRMWARE_UPDATE_STATUS_VALUES);

/* Paste in your device connection string  */
static const char* connectionString = "<connectionstring>";
static char msgText[1024];
static size_t g_message_count_send_confirmations = 0;
static const char* initialFirmwareVersion = "1.0.0";

IOTHUB_DEVICE_CLIENT_HANDLE device_handle;

// <datadefinition>
typedef struct MESSAGESCHEMA_TAG
{
	char* name;
	char* format;
	char* fields;
} MessageSchema;

typedef struct TELEMETRYSCHEMA_TAG
{
	MessageSchema messageSchema;
} TelemetrySchema;

typedef struct TELEMETRYPROPERTIES_TAG
{
	TelemetrySchema temperatureSchema;
	TelemetrySchema humiditySchema;
	TelemetrySchema pressureSchema;
} TelemetryProperties;

typedef struct CHILLER_TAG
{
	// Reported properties
	char* protocol;
	char* supportedMethods;
	char* type;
	char* firmware;
	FIRMWARE_UPDATE_STATUS firmwareUpdateStatus;
	char* location;
	double latitude;
	double longitude;
	TelemetryProperties telemetry;

	// Manage firmware update process
	char* new_firmware_version;
	char* new_firmware_URI;
} Chiller;
// </datadefinition>

/*  Converts the Chiller object into a JSON blob with reported properties ready to be sent across the wire as a twin. */
static char* serializeToJson(Chiller* chiller)
{
	char* result;

	JSON_Value* root_value = json_value_init_object();
	JSON_Object* root_object = json_value_get_object(root_value);

	// Only reported properties:
	(void)json_object_set_string(root_object, "Protocol", chiller->protocol);
	(void)json_object_set_string(root_object, "SupportedMethods", chiller->supportedMethods);
	(void)json_object_set_string(root_object, "Type", chiller->type);
	(void)json_object_set_string(root_object, "Firmware", chiller->firmware);
	(void)json_object_set_string(root_object, "FirmwareUpdateStatus", MU_ENUM_TO_STRING(FIRMWARE_UPDATE_STATUS, chiller->firmwareUpdateStatus));
	(void)json_object_set_string(root_object, "Location", chiller->location);
	(void)json_object_set_number(root_object, "Latitude", chiller->latitude);
	(void)json_object_set_number(root_object, "Longitude", chiller->longitude);
	(void)json_object_dotset_string(root_object, "Telemetry.TemperatureSchema.MessageSchema.Name", chiller->telemetry.temperatureSchema.messageSchema.name);
	(void)json_object_dotset_string(root_object, "Telemetry.TemperatureSchema.MessageSchema.Format", chiller->telemetry.temperatureSchema.messageSchema.format);
	(void)json_object_dotset_string(root_object, "Telemetry.TemperatureSchema.MessageSchema.Fields", chiller->telemetry.temperatureSchema.messageSchema.fields);
	(void)json_object_dotset_string(root_object, "Telemetry.HumiditySchema.MessageSchema.Name", chiller->telemetry.humiditySchema.messageSchema.name);
	(void)json_object_dotset_string(root_object, "Telemetry.HumiditySchema.MessageSchema.Format", chiller->telemetry.humiditySchema.messageSchema.format);
	(void)json_object_dotset_string(root_object, "Telemetry.HumiditySchema.MessageSchema.Fields", chiller->telemetry.humiditySchema.messageSchema.fields);
	(void)json_object_dotset_string(root_object, "Telemetry.PressureSchema.MessageSchema.Name", chiller->telemetry.pressureSchema.messageSchema.name);
	(void)json_object_dotset_string(root_object, "Telemetry.PressureSchema.MessageSchema.Format", chiller->telemetry.pressureSchema.messageSchema.format);
	(void)json_object_dotset_string(root_object, "Telemetry.PressureSchema.MessageSchema.Fields", chiller->telemetry.pressureSchema.messageSchema.fields);

	result = json_serialize_to_string(root_value);

	json_value_free(root_value);

	return result;
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
	(void)reason;
	(void)user_context;
	// This sample DOES NOT take into consideration network outages.
	if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
	{
		(void)printf("The device client is connected to iothub\r\n");
	}
	else
	{
		(void)printf("The device client has been disconnected\r\n");
	}
}

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
	(void)userContextCallback;
	g_message_count_send_confirmations++;
	(void)printf("Confirmation callback received for message %zu with result %s\r\n", g_message_count_send_confirmations, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void reported_state_callback(int status_code, void* userContextCallback)
{
	(void)userContextCallback;
	(void)printf("Device Twin reported properties update completed with result: %d\r\n", status_code);
}

static void sendChillerReportedProperties(Chiller* chiller)
{
	if (device_handle != NULL)
	{
		char* reportedProperties = serializeToJson(chiller);
		(void)IoTHubDeviceClient_SendReportedState(device_handle, (const unsigned char*)reportedProperties, strlen(reportedProperties), reported_state_callback, NULL);
		free(reportedProperties);
	}
}

// <firmwareupdate>
/*
 This is a thread allocated to process a long-running device method call.
 It uses device twin reported properties to communicate status values
 to the Remote Monitoring solution accelerator.
*/
static int do_firmware_update(void *param)
{
	Chiller *chiller = (Chiller *)param;
	printf("Running simulated firmware update: URI: %s, Version: %s\r\n", chiller->new_firmware_URI, chiller->new_firmware_version);

	printf("Simulating download phase...\r\n");
	chiller->firmwareUpdateStatus = DOWNLOADING;
	sendChillerReportedProperties(chiller);

	ThreadAPI_Sleep(5000);

	printf("Simulating apply phase...\r\n");
	chiller->firmwareUpdateStatus = APPLYING;
	sendChillerReportedProperties(chiller);

	ThreadAPI_Sleep(5000);

	printf("Simulating reboot phase...\r\n");
	chiller->firmwareUpdateStatus = REBOOTING;
	sendChillerReportedProperties(chiller);

	ThreadAPI_Sleep(5000);

	size_t size = strlen(chiller->new_firmware_version) + 1;
	(void)memcpy(chiller->firmware, chiller->new_firmware_version, size);

	chiller->firmwareUpdateStatus = IDLE;
	sendChillerReportedProperties(chiller);

	return 0;
}
// </firmwareupdate>

void getFirmwareUpdateValues(Chiller* chiller, const unsigned char* payload)
{
	free(chiller->new_firmware_version);
	free(chiller->new_firmware_URI);
	chiller->new_firmware_URI = NULL;
	chiller->new_firmware_version = NULL;

	JSON_Value* root_value = json_parse_string((char *)payload);
	JSON_Object* root_object = json_value_get_object(root_value);

	JSON_Value* newFirmwareVersion = json_object_get_value(root_object, "Firmware");

	if (newFirmwareVersion != NULL)
	{
		const char* data = json_value_get_string(newFirmwareVersion);
		if (data != NULL)
		{
			size_t size = strlen(data) + 1;
			chiller->new_firmware_version = malloc(size);
			if (chiller->new_firmware_version != NULL)
			{
				(void)memcpy(chiller->new_firmware_version, data, size);
			}
		}
	}

	JSON_Value* newFirmwareURI = json_object_get_value(root_object, "FirmwareUri");

	if (newFirmwareURI != NULL)
	{
		const char* data = json_value_get_string(newFirmwareURI);
		if (data != NULL)
		{
			size_t size = strlen(data) + 1;
			chiller->new_firmware_URI = malloc(size);
			if (chiller->new_firmware_URI != NULL)
			{
				(void)memcpy(chiller->new_firmware_URI, data, size);
			}
		}
	}

	// Free resources
	json_value_free(root_value);

}

// <devicemethodcallback>
static int device_method_callback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
	Chiller *chiller = (Chiller *)userContextCallback;

	int result;

	(void)printf("Direct method name:    %s\r\n", method_name);

	(void)printf("Direct method payload: %.*s\r\n", (int)size, (const char*)payload);

	if (strcmp("Reboot", method_name) == 0)
	{
		MESSAGERESPONSE(201, "{ \"Response\": \"Rebooting\" }")
	}
	else if (strcmp("EmergencyValveRelease", method_name) == 0)
	{
		MESSAGERESPONSE(201, "{ \"Response\": \"Releasing emergency valve\" }")
	}
	else if (strcmp("IncreasePressure", method_name) == 0)
	{
		MESSAGERESPONSE(201, "{ \"Response\": \"Increasing pressure\" }")
	}
	else if (strcmp("FirmwareUpdate", method_name) == 0)
	{
		if (chiller->firmwareUpdateStatus != IDLE)
		{
			(void)printf("Attempt to invoke firmware update out of order\r\n");
			MESSAGERESPONSE(400, "{ \"Response\": \"Attempting to initiate a firmware update out of order\" }")
		}
		else
		{
			getFirmwareUpdateValues(chiller, payload);

			if (chiller->new_firmware_version != NULL && chiller->new_firmware_URI != NULL)
			{
				// Create a thread for the long-running firmware update process.
				THREAD_HANDLE thread_apply;
				THREADAPI_RESULT t_result = ThreadAPI_Create(&thread_apply, do_firmware_update, chiller);
				if (t_result == THREADAPI_OK)
				{
					(void)printf("Starting firmware update thread\r\n");
					MESSAGERESPONSE(201, "{ \"Response\": \"Starting firmware update thread\" }")
				}
				else
				{
					(void)printf("Failed to start firmware update thread\r\n");
					MESSAGERESPONSE(500, "{ \"Response\": \"Failed to start firmware update thread\" }")
				}
			}
			else
			{
				(void)printf("Invalid method payload\r\n");
				MESSAGERESPONSE(400, "{ \"Response\": \"Invalid payload\" }")
			}
		}
	}
	else
	{
		// All other entries are ignored.
		(void)printf("Method not recognized\r\n");
		MESSAGERESPONSE(400, "{ \"Response\": \"Method not recognized\" }")
	}

	return result;
}
// </devicemethodcallback>

// <sendmessage>
static void send_message(IOTHUB_DEVICE_CLIENT_HANDLE handle, char* message, char* schema)
{
	IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString(message);

	// Set system properties
	(void)IoTHubMessage_SetMessageId(message_handle, "MSG_ID");
	(void)IoTHubMessage_SetCorrelationId(message_handle, "CORE_ID");
	(void)IoTHubMessage_SetContentTypeSystemProperty(message_handle, "application%2fjson");
	(void)IoTHubMessage_SetContentEncodingSystemProperty(message_handle, "utf-8");

	// Set application properties
	MAP_HANDLE propMap = IoTHubMessage_Properties(message_handle);
	(void)Map_AddOrUpdate(propMap, "$$MessageSchema", schema);
	(void)Map_AddOrUpdate(propMap, "$$ContentType", "JSON");

	time_t now = time(0);
	struct tm* timeinfo;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996) /* Suppress warning about possible unsafe function in Visual Studio */
#endif
	timeinfo = gmtime(&now);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	char timebuff[50];
	strftime(timebuff, 50, "%Y-%m-%dT%H:%M:%SZ", timeinfo);
	(void)Map_AddOrUpdate(propMap, "$$CreationTimeUtc", timebuff);

	IoTHubDeviceClient_SendEventAsync(handle, message_handle, send_confirm_callback, NULL);

	IoTHubMessage_Destroy(message_handle);
}
// </sendmessage>

// <main>
int main(void)
{
	srand((unsigned int)time(NULL));
	double minTemperature = 50.0;
	double minPressure = 55.0;
	double minHumidity = 30.0;
	double temperature = 0;
	double pressure = 0;
	double humidity = 0;

	(void)printf("This sample simulates a Chiller device connected to the Remote Monitoring solution accelerator\r\n\r\n");

	// Used to initialize sdk subsystem
	(void)IoTHub_Init();

	(void)printf("Creating IoTHub handle\r\n");
	// Create the iothub handle here
	device_handle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, MQTT_Protocol);
	if (device_handle == NULL)
	{
		(void)printf("Failure creating IotHub device. Hint: Check your connection string.\r\n");
	}
	else
	{
		// Setting connection status callback to get indication of connection to iothub
		(void)IoTHubDeviceClient_SetConnectionStatusCallback(device_handle, connection_status_callback, NULL);

		Chiller chiller;
		memset(&chiller, 0, sizeof(Chiller));
		chiller.protocol = "MQTT";
		chiller.supportedMethods = "Reboot,FirmwareUpdate,EmergencyValveRelease,IncreasePressure";
		chiller.type = "Chiller";
		size_t size = strlen(initialFirmwareVersion) + 1;
		chiller.firmware = malloc(size);
		if (chiller.firmware == NULL)
		{
			(void)printf("Chiller Firmware failed to allocate memory.\r\n");
		}
		else
		{
			memcpy(chiller.firmware, initialFirmwareVersion, size);
			chiller.firmwareUpdateStatus = IDLE;
			chiller.location = "Building 44";
			chiller.latitude = 47.638928;
			chiller.longitude = -122.13476;
			chiller.telemetry.temperatureSchema.messageSchema.name = "chiller-temperature;v1";
			chiller.telemetry.temperatureSchema.messageSchema.format = "JSON";
			chiller.telemetry.temperatureSchema.messageSchema.fields = "{\"temperature\":\"Double\",\"temperature_unit\":\"Text\"}";
			chiller.telemetry.humiditySchema.messageSchema.name = "chiller-humidity;v1";
			chiller.telemetry.humiditySchema.messageSchema.format = "JSON";
			chiller.telemetry.humiditySchema.messageSchema.fields = "{\"humidity\":\"Double\",\"humidity_unit\":\"Text\"}";
			chiller.telemetry.pressureSchema.messageSchema.name = "chiller-pressure;v1";
			chiller.telemetry.pressureSchema.messageSchema.format = "JSON";
			chiller.telemetry.pressureSchema.messageSchema.fields = "{\"pressure\":\"Double\",\"pressure_unit\":\"Text\"}";

			sendChillerReportedProperties(&chiller);

			(void)IoTHubDeviceClient_SetDeviceMethodCallback(device_handle, device_method_callback, &chiller);

			while (1)
			{
				temperature = minTemperature + ((rand() % 10) + 5);
				pressure = minPressure + ((rand() % 10) + 5);
				humidity = minHumidity + ((rand() % 20) + 5);

				if (chiller.firmwareUpdateStatus == IDLE)
				{
					(void)printf("Sending sensor value Temperature = %f %s,\r\n", temperature, "F");
					(void)sprintf_s(msgText, sizeof(msgText), "{\"temperature\":%.2f,\"temperature_unit\":\"F\"}", temperature);
					send_message(device_handle, msgText, chiller.telemetry.temperatureSchema.messageSchema.name);


					(void)printf("Sending sensor value Pressure = %f %s,\r\n", pressure, "psig");
					(void)sprintf_s(msgText, sizeof(msgText), "{\"pressure\":%.2f,\"pressure_unit\":\"psig\"}", pressure);
					send_message(device_handle, msgText, chiller.telemetry.pressureSchema.messageSchema.name);


					(void)printf("Sending sensor value Humidity = %f %s,\r\n", humidity, "%");
					(void)sprintf_s(msgText, sizeof(msgText), "{\"humidity\":%.2f,\"humidity_unit\":\"%%\"}", humidity);
					send_message(device_handle, msgText, chiller.telemetry.humiditySchema.messageSchema.name);
				}

				ThreadAPI_Sleep(5000);
			}

			(void)printf("\r\nShutting down\r\n");

			// Clean up the iothub sdk handle and free resources
			IoTHubDeviceClient_Destroy(device_handle);
			free(chiller.firmware);
			free(chiller.new_firmware_URI);
			free(chiller.new_firmware_version);
		}
	}
	// Shutdown the sdk subsystem
	IoTHub_Deinit();

	return 0;
}
// </main>
