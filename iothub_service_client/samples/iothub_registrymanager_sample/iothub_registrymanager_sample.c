// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"

#include "iothub_service_client_auth.h"
#include "iothub_registrymanager.h"

#define USE_DEPRECATED 1

typedef enum COMMAND_ID_TAG
{
	UNKNOWN = -1,
	CREATE_DEVICE = 0,
	READ_DEVICE,
	UPDATE_DEVICE,
	DELETE_DEVICE,
	LIST_DEVICE,
	GET_STATISTICS,
	NUM_OF_COMAMNDS
} COMMAND_ID;

typedef enum OPTION_ARG_NO_TAG
{
	CONNECTION_STRING = 0,
	DEVICE_ID,
	KEY_1,
	KEY_2,
	NUM_OF_OPTION_ARGS
} OPTION_ARG_NO;

typedef struct MANAGER_OPTIONS_TAG
{
	COMMAND_ID commandId;
	const char* commandName;
	int optionNum;
	int keyOption;
} MANAGER_OPTIONS;

typedef struct IOTHUB_DEVICE_INTERNAL_TAG
{
	/*int type;*/
	const char* deviceId;
	const char* primaryKey;
	const char* secondaryKey;
	const char* generationId;
	const char* eTag;
	IOTHUB_DEVICE_CONNECTION_STATE connectionState;
	const char* connectionStateUpdatedTime;
	IOTHUB_DEVICE_STATUS status;
	const char* statusReason;
	const char* statusUpdatedTime;
	const char* lastActivityTime;
	size_t cloudToDeviceMessageCount;

	bool isManaged;
	const char* configuration;
	const char* deviceProperties;
	const char* serviceProperties;
	IOTHUB_REGISTRYMANAGER_AUTH_METHOD authMethod;

	//Device exclusive fields
	bool iotEdge_capable;

	//Module exclusive fields
	const char* moduleId;
	const char* managedBy;
} IOTHUB_DEVICE_INTERNAL;

static const MANAGER_OPTIONS managerOption[] = {
	/* CMD_ID cmd_name, optnum, key opts */
	{CREATE_DEVICE, "create", 1, true },
	{READ_DEVICE, "read", 1, false },
	{READ_DEVICE, "get", 1, false },
	{UPDATE_DEVICE, "update", 1, true },
	{DELETE_DEVICE, "delete", 1, false },
#if USE_DEPRECATED
	{LIST_DEVICE, "list", 0, false },
#endif
	{GET_STATISTICS, "statistics", 0, false },
	{UNKNOWN, NULL, 0, false }
};

typedef struct OPTION_ARGS_TAG
{
	const char* optionName;
	OPTION_ARG_NO optionNo;
} OPTION_ARGS;

static OPTION_ARGS optionList[] = {
	{"-l", CONNECTION_STRING},
	{"--login", CONNECTION_STRING},
	{"-k1", KEY_1},
	{"--key1", KEY_1},
	{"-k2", KEY_2},
	{"--key2", KEY_2},
	{NULL, UNKNOWN}
};

static char *myName;

int checkPreOptions(int *argc, char ***argv)
{
	int helpCount = 0;

	while (*argc > 0 && *argv[0][0] == '-')
	{
		if (0 == strcmp("-h", *argv[0])
		 || 0 == strcmp("--help", *argv[0])
		 || 0 == strcmp("-v", *argv[0])
		 || 0 == strcmp("--version", *argv[0]))
		{
			helpCount++;
		}
		else
		{
			(void)fprintf(stderr, "Unknown option: %s\n\n", *argv[0]);
			helpCount++;
			break;
		}
		(*argc)--;
		(*argv)++;
	}
	return helpCount;
}

int checkCommand(char *command)
{
	int i;

	for (i = 0; managerOption[i].commandName != NULL; i++)
	{
		if (0 == strcmp(managerOption[i].commandName, command))
		{
			return i;
		}
	}
	return UNKNOWN;
}

int getOptionArgs(int cmdIndex, int *argc, char ***argv, char **args)
{
	OPTION_ARGS *table;
	int i;
	int csCount = 0;

	while (*argc > 1 && *argv[0][0] == '-')
	{
		if (managerOption[cmdIndex].keyOption)
		{
			// Get Keys or CS
			table = &optionList[0];
			for (i = 0; table->optionName != NULL; i++)
			{
				if (0 == strcmp(table->optionName, *argv[0]))
				{
					(*argc)--;
					(*argv)++;
					if (argc > 0 && *argv[0] != NULL)
					{
						args[table->optionNo] = *argv[0];
						if (table->optionNo == CONNECTION_STRING)
						{
							csCount++;
						}
						(*argc)--;
						(*argv)++;
						break;
					}
					else
					{
						csCount = 0;
						break;
					}
				}
				table++;
			}
			if (NULL == table->optionName)
			{
				(void)fprintf(stderr, "Unknown option=%s\n\n", *argv[0]);
				csCount = 0;
				break;
			}
		}
		else if (0 == strcmp("-l", *argv[0]) || 0 == strcmp("--login", *argv[0]))
		{
			(*argc)--;
			(*argv)++;
			if (*argc > 0)
			{
				args[CONNECTION_STRING] = *argv[0];
				csCount++;
				(*argc)--;
				(*argv)++;
			}
			else
			{
				(void)fprintf(stderr, "Connection-string needed\n\n");
				csCount = 0;
				break;
			}
		}
		else
		{
			(void)fprintf(stderr, "Unknown option:%s\n\n", *argv[0]);
			csCount = 0;
			break;
		}
	} //while()

	if (managerOption[cmdIndex].optionNum > 0)
	{
		if (*argc > 0)
		{
			args[DEVICE_ID] = *argv[0];
			(*argc)--;
			(*argv)++;
		}
		else
		{
			(void)fprintf(stderr, "Device_ID needed\n\n");
			csCount = 0;
		}
	}
	return csCount;
}

static char* connectionString;
static char* deviceId = "";
static char* primaryKey = "";
static char* secondaryKey = "";

static void printDeviceInfo(const IOTHUB_DEVICE_EX* device, int orderNum)
{
    if ((device != NULL) && (device->deviceId != NULL))
    {
        if (orderNum >= 0)
        {
            (void)printf("Device(%d)\n", orderNum);
        }
        else
        {
            (void)printf("Device\n");
        }
        (void)printf("    deviceId                    : %s\n", device->deviceId);
        (void)printf("    primaryKey                  : %s\n", device->primaryKey);
        (void)printf("    secondaryKey                : %s\n", device->secondaryKey);
        (void)printf("    generationId                : %s\n", device->generationId);
        (void)printf("    eTag                        : %s\n", device->eTag);
        (void)printf("    authMethod                  : %d\n", device->authMethod);

        if (device->connectionState == IOTHUB_DEVICE_CONNECTION_STATE_CONNECTED)
        {
            (void)printf("    connectionState             : Connected\n");
        }
        else
        {
            (void)printf("    connectionState             : Disconnected\n");
        }
        (void)printf("    connectionStateUpdatedTime  : %s\n", device->connectionStateUpdatedTime);
        if (device->status == IOTHUB_DEVICE_STATUS_ENABLED)
        {
            (void)printf("    status                      : Enabled\n");
        }
        else
        {
            (void)printf("    status                      : Disabled\n");
        }
        (void)printf("    statusReason                : %s\n", device->statusReason);
        (void)printf("    statusUpdatedTime           : %s\n", device->statusUpdatedTime);
        (void)printf("    lastActivityTime            : %s\n", device->lastActivityTime);
        (void)printf("    cloudToDeviceMessageCount   : %zu\n", device->cloudToDeviceMessageCount);
    }
}

static void free_device_resource(IOTHUB_DEVICE_EX* device)
{
    free((char*)device->deviceId);
    free((char*)device->primaryKey);
    free((char*)device->secondaryKey);
    free((char*)device->generationId);
    free((char*)device->eTag);
    free((char*)device->connectionStateUpdatedTime);
    free((char*)device->statusReason);
    free((char*)device->statusUpdatedTime);
    free((char*)device->lastActivityTime);
    free((char*)device->configuration);
    free((char*)device->deviceProperties);
    free((char*)device->serviceProperties);
}

void usage()
{
	(void)printf("Usage: %s [options] <command> [command-options] [command-args]\n\n", myName);
	(void)printf("Options:\n\n");
	(void)printf("  -V, --version                   output the version number\n");
	(void)printf("  -h, --help                      output usage information\n\n");
	(void)printf("Commands:\n\n");
	(void)printf("  create <device-id>              create a device identity in your IoT hub device registry\n");
	(void)printf("  read <device-id>                get a device identity from your IoT hub device registry\n");
	(void)printf("  get <device-id>                 get a device identity from your IoT hub device registry\n");
	(void)printf("  update <device-id>              update device key[s] in your IoT hub device registry\n");
	(void)printf("  delete <device-id>              delete a device identity from your IoT hub device registry\n");
#if USE_DEPRECATED
	(void)printf("  list                            list the device identities currently in your IoT hub device registry\n");
#endif
	(void)printf("  statistic                       get a IoT Hub statistics\n\n");
	(void)printf("Command-options:\n\n");
	(void)printf("  -l, --login <connection-string> connection string to use to authenticate with your IoT Hub instance\n");
	(void)printf("  -k1, --key1 <key>               specify the primary key for newly created device\n");
	(void)printf("  -k2, --key2 <key>               specify the secondary key for newly created device\n");
}

int main(int argc, char **argv)
{
	int cmdIndex;
	char *optionArgs[NUM_OF_OPTION_ARGS] = { "", "", "", "" };
	int i = 0, option;

	myName = argv[0];
	--argc;
	++argv;
	if (argc > 0)
	{
		if (checkPreOptions(&argc, &argv) > 0)
		{
			usage();
			return 0;
		}
		if (argc <= 0) {
			usage();
			return 0;
		}
		cmdIndex = checkCommand(argv[0]);
		if (cmdIndex == UNKNOWN) {
			(void)fprintf(stderr, "Unknown command=%s\n\n", argv[0]);
			usage();
			return 0;
		}

		--argc;
		++argv;
		if (0 == getOptionArgs(cmdIndex, &argc, &argv, optionArgs))
		{
			usage();
			return 0;
		}
	}
	else
	{
		usage();
		return 0;
	}

	option = (int) managerOption[cmdIndex].commandId;
	connectionString = optionArgs[CONNECTION_STRING];
	deviceId = optionArgs[DEVICE_ID];
	primaryKey = optionArgs[KEY_1];
	secondaryKey = optionArgs[KEY_2];

    (void)platform_init();

    IOTHUB_REGISTRYMANAGER_RESULT result;

    (void)printf("Calling IoTHubServiceClientAuth_CreateFromConnectionString with the connection string\n");
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iotHubServiceClientHandle = IoTHubServiceClientAuth_CreateFromConnectionString(connectionString);
    if (iotHubServiceClientHandle == NULL)
    {
        (void)printf("IoTHubServiceClientAuth_CreateFromConnectionString failed\n");
		platform_deinit();
		return 0;
    }

	IOTHUB_REGISTRY_DEVICE_CREATE_EX deviceCreateInfo;
    (void)printf("Creating device with RegistryManager...\n");
    IOTHUB_REGISTRYMANAGER_HANDLE iotHubRegistryManagerHandle = IoTHubRegistryManager_Create(iotHubServiceClientHandle);

    (void)memset(&deviceCreateInfo, 0, sizeof(IOTHUB_REGISTRY_DEVICE_CREATE_EX));
    deviceCreateInfo.deviceId = deviceId;
    deviceCreateInfo.primaryKey = primaryKey;
    deviceCreateInfo.secondaryKey = secondaryKey;
    deviceCreateInfo.authMethod = IOTHUB_REGISTRYMANAGER_AUTH_SPK;
    deviceCreateInfo.version = IOTHUB_REGISTRY_DEVICE_CREATE_EX_VERSION_1;

    IOTHUB_DEVICE_EX deviceInfo;
	IOTHUB_REGISTRY_DEVICE_UPDATE deviceUpdateInfo;
    IOTHUB_REGISTRY_STATISTICS registryStatistics;

	deviceInfo.version = IOTHUB_DEVICE_EX_VERSION_1;
	
    switch(option)
    {
	case CREATE_DEVICE:
        // Create device
        result = IoTHubRegistryManager_CreateDevice_Ex(iotHubRegistryManagerHandle, &deviceCreateInfo, &deviceInfo);
        if (result == IOTHUB_REGISTRYMANAGER_OK)
        {
            (void)printf("IoTHubRegistryManager_CreateDevice: Device has been created successfully: deviceId=%s\n", deviceInfo.deviceId);
        }
        else if (result == IOTHUB_REGISTRYMANAGER_DEVICE_EXIST)
        {
            (void)printf("IoTHubRegistryManager_CreateDevice: Device already exists\n");
        }
        else if (result == IOTHUB_REGISTRYMANAGER_ERROR)
        {
            (void)printf("IoTHubRegistryManager_CreateDevice failed\n");
        }
        // You will need to Free the returned device information after it was created
        free_device_resource(&deviceInfo);
        break;

    case UPDATE_DEVICE:
        // Update device
        (void)memset(&deviceUpdateInfo, 0, sizeof(IOTHUB_REGISTRY_DEVICE_UPDATE));

        deviceUpdateInfo.deviceId = deviceId;
		deviceUpdateInfo.primaryKey = primaryKey;
		deviceUpdateInfo.secondaryKey = secondaryKey;
        deviceUpdateInfo.authMethod = IOTHUB_REGISTRYMANAGER_AUTH_SPK;
        deviceUpdateInfo.status = IOTHUB_DEVICE_STATUS_DISABLED;
        result = IoTHubRegistryManager_UpdateDevice(iotHubRegistryManagerHandle, &deviceUpdateInfo);
        if (result == IOTHUB_REGISTRYMANAGER_OK)
        {
            (void)printf("IoTHubRegistryManager_UpdateDevice: Device has been updated successfully: deviceId=%s\n", deviceUpdateInfo.deviceId);
        }
        else if (result == IOTHUB_REGISTRYMANAGER_ERROR)
        {
            (void)printf("IoTHubRegistryManager_UpdateDevice failed\n");
        }
        break;

#if USE_DEPRECATED
    case LIST_DEVICE:
	    do
	    {
	 		SINGLYLINKEDLIST_HANDLE deviceList = singlylinkedlist_create();
	 		LIST_ITEM_HANDLE next_device;

	 		result = IoTHubRegistryManager_GetDeviceList(iotHubRegistryManagerHandle, 1000, deviceList);

	 		switch (result)
	 		{
	 		case IOTHUB_REGISTRYMANAGER_OK:
	 			(void)printf("IoTHubRegistryManager_GetDeviceList: Successfully got device list\r\n");

	 			next_device = singlylinkedlist_get_head_item(deviceList);
	 			i = 0;
	 			while (next_device != NULL)
	 			{
					IOTHUB_DEVICE_INTERNAL* devmod = (IOTHUB_DEVICE_INTERNAL*)singlylinkedlist_item_get_value(next_device);
	 				IOTHUB_DEVICE_EX Device;
					IOTHUB_DEVICE_EX *device = &Device;
					device->deviceId = devmod->deviceId;
					device->primaryKey = devmod->primaryKey;
					device->secondaryKey = devmod->secondaryKey;
					device->generationId = devmod->generationId;
					device->eTag = devmod->eTag; 
					device->connectionStateUpdatedTime = devmod->connectionStateUpdatedTime;
					device->statusReason = devmod->statusReason;
					device->statusUpdatedTime = devmod->statusUpdatedTime;
					device->lastActivityTime = devmod->lastActivityTime;
					device->configuration = devmod->configuration;
					device->deviceProperties = devmod->deviceProperties;
					device->serviceProperties = devmod->serviceProperties;

					printDeviceInfo(device, i++);
					next_device = singlylinkedlist_get_next_item(next_device);
				}
				break;
			
			case IOTHUB_REGISTRYMANAGER_ERROR:
				 (void)printf("IoTHubRegistryManager_GetDeviceList failed\n");
				break;

			default:
				(void)printf("IoTHubRegistryManager_GetDeviceList failed with unknown error\n");
				break;
			}
		}
		while(0);
        break;
#endif

    case READ_DEVICE:
        // Get device
        (void)memset(&deviceInfo, 0, sizeof(IOTHUB_DEVICE_EX));
        deviceInfo.version = IOTHUB_REGISTRY_DEVICE_CREATE_EX_VERSION_1;
		result = IoTHubRegistryManager_GetDevice_Ex(iotHubRegistryManagerHandle, deviceCreateInfo.deviceId, &deviceInfo);
		if (result == IOTHUB_REGISTRYMANAGER_OK)
		{
			(void)printf("%d: IoTHubRegistryManager_GetDevice: Successfully got device info: deviceId=%s\n", i, deviceInfo.deviceId);
			printDeviceInfo(&deviceInfo, i);
		}
		else if (result == IOTHUB_REGISTRYMANAGER_ERROR)
		{
			(void)printf("IoTHubRegistryManager_GetDevice failed\n");
		}
        // You will need to free the device information getting the devic
        free_device_resource(&deviceInfo);
        break;

    case DELETE_DEVICE:	
        // Delete device
        result = IoTHubRegistryManager_DeleteDevice(iotHubRegistryManagerHandle, deviceCreateInfo.deviceId);
        if (result == IOTHUB_REGISTRYMANAGER_OK)
        {
            (void)printf("IoTHubRegistryManager_DeleteDevice: Device has been deleted successfully: deviceId=%s\n", deviceCreateInfo.deviceId);
        }
        else if (result == IOTHUB_REGISTRYMANAGER_ERROR)
        {
            (void)printf("IoTHubRegistryManager_DeleteDevice: Delete device failed\n");
        }
        break;

    case GET_STATISTICS:	
        // Get statistics
        result = IoTHubRegistryManager_GetStatistics(iotHubRegistryManagerHandle, &registryStatistics);
        if (result == IOTHUB_REGISTRYMANAGER_OK)
        {
            (void)printf("IoTHubRegistryManager_GetStatistics: Successfully got registry statistics\n");
            (void)printf("Total device count: %zu\n", registryStatistics.totalDeviceCount);
            (void)printf("Enabled device count: %zu\n", registryStatistics.enabledDeviceCount);
            (void)printf("Disabled device count: %zu\n", registryStatistics.disabledDeviceCount);
        }
        else if (result == IOTHUB_REGISTRYMANAGER_ERROR)
        {
            (void)printf("IoTHubRegistryManager_GetStatistics failed\n");
        }

        (void)printf("Calling IoTHubRegistryManager_Destroy...\n");
        IoTHubRegistryManager_Destroy(iotHubRegistryManagerHandle);

        (void)printf("Calling IoTHubServiceClientAuth_Destroy...\n");
        IoTHubServiceClientAuth_Destroy(iotHubServiceClientHandle);
        break;
	}
    platform_deinit();
	return 0;
}
