// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#endif

#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "iothub_account.h"

#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/azure_base64.h"
#include "azure_c_shared_utility/hmacsha256.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/sastoken.h"

#include "azure_c_shared_utility/httpapiexsas.h"
#include "azure_c_shared_utility/uniqueid.h"

#include "iothub_service_client_auth.h"
#include "iothub_registrymanager.h"

#ifdef MBED_BUILD_TIMESTAMP
#define MBED_PARAM_MAX_LENGTH 256
#endif

static const char* DEVICE_JSON_FMT = "{\"deviceId\":\"%s\",\"etag\":null,\"connectionState\":\"Disconnected\",\"status\":\"enabled\",\"statusReason\":null,\"connectionStateUpdatedTime\":\"0001-01-01T00:00:00\",\"statusUpdatedTime\":\"0001-01-01T00:00:00\",\"lastActivityTime\":\"0001-01-01T00:00:00\",\"authentication\":{\"symmetricKey\":{\"primaryKey\":null,\"secondaryKey\":null}}}";
static const char* SAS_DEVICE_PREFIX_FMT = "csdk_e2eDevice_sas_j_please_delete_%s";
static const char* X509_DEVICE_PREFIX_FMT = "csdk_e2eDevice_x509_j_please_delete_%s";
static const char* RELATIVE_PATH_FMT = "/devices/%s?%s";
static const char* SHARED_ACCESS_KEY = "SharedAccessSignature sr=%s&sig=%s&se=%s&skn=%s";

static const char *DEFAULT_CONSUMER_GROUP = "$Default";
static const int DEFAULT_PARTITION_COUNT = 16;

static const char* PRIMARY_KEY_FIELD = "\"primaryKey\":\"";
static const size_t PRIMARY_KEY_FIELD_LEN = 14;

static const char* TEST_MODULE_NAME = "TestModule";
static const char* TEST_MANAGED_BY_1 = "TestManagedBy1";
static const char* TEST_MANAGED_BY_2 = "TestManagedBy2";




#define MAX_LENGTH_OF_UNSIGNED_INT  6
#define MAX_LENGTH_OF_TIME_VALUE    12
#define MAX_RAND_VALUE              10000
#define DEVICE_GUID_SIZE            37

typedef struct IOTHUB_ACCOUNT_INFO_TAG
{
    const char* connString;
    const char* eventhubConnString;
    char* hostname;
    char* iothubName;
    char* iothubSuffix;
    char* sharedAccessKey;
    char* sharedAccessToken;
    char* keyName;
    char* eventhubAccessKey;
    char* x509Certificate;
    char* x509PrivateKey;
    char* x509Thumbprint;
    size_t number_of_sas_devices;
    IOTHUB_PROVISIONED_DEVICE** sasDevices;
    IOTHUB_PROVISIONED_DEVICE x509Device;
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE iothub_service_client_auth_handle;
    IOTHUB_REGISTRYMANAGER_HANDLE iothub_registrymanager_handle;
    IOTHUB_MESSAGING_HANDLE iothub_messaging_handle;
} IOTHUB_ACCOUNT_INFO;

static HTTP_HEADERS_HANDLE getContentHeaders(bool appendIfMatch)
{
    HTTP_HEADERS_HANDLE httpHeader = HTTPHeaders_Alloc();
    if (httpHeader != NULL)
    {
        if (HTTPHeaders_AddHeaderNameValuePair(httpHeader, "Authorization", " ") != HTTP_HEADERS_OK ||
            HTTPHeaders_AddHeaderNameValuePair(httpHeader, "User-Agent", "Microsoft.Azure.Devices/1.0.0") != HTTP_HEADERS_OK ||
            HTTPHeaders_AddHeaderNameValuePair(httpHeader, "Accept", "application/json") != HTTP_HEADERS_OK ||
            HTTPHeaders_AddHeaderNameValuePair(httpHeader, "Content-Type", "application/json; charset=utf-8") != HTTP_HEADERS_OK)
        {
            LogError("Failure adding http headers.\r\n");
            HTTPHeaders_Free(httpHeader);
            httpHeader = NULL;
        }
        else
        {
            if (appendIfMatch)
            {
                if (HTTPHeaders_AddHeaderNameValuePair(httpHeader, "If-Match", "*") != HTTP_HEADERS_OK)
                {
                    LogError("Failure adding if-Match http headers.\r\n");
                    HTTPHeaders_Free(httpHeader);
                    httpHeader = NULL;
                }
            }
        }
    }
    return httpHeader;
}

static int generateDeviceName(const char* prefix, char** deviceName)
{
    int result;
    char deviceGuid[DEVICE_GUID_SIZE];
    if (UniqueId_Generate(deviceGuid, DEVICE_GUID_SIZE) != UNIQUEID_OK)
    {
        LogError("Unable to generate unique Id.\r\n");
        result = MU_FAILURE;
    }
    else
    {
        size_t len = strlen(prefix) + DEVICE_GUID_SIZE;
        *deviceName = (char*)malloc(len + 1);
        if (*deviceName == NULL)
        {
            LogError("Failure allocating device ID.\r\n");
            result = MU_FAILURE;
        }
        else
        {
            if (sprintf_s(*deviceName, len + 1, prefix, deviceGuid) <= 0)
            {
                LogError("Failure constructing device ID.\r\n");
                free(*deviceName);
                result = MU_FAILURE;
            }
            else
            {
                LogInfo("Created Device %s.", *deviceName);
                result = 0;
            }
        }
    }
    return result;
}

static int retrieveConnStringInfo(IOTHUB_ACCOUNT_INFO* accountInfo)
{
    int result;
    int beginName, endName, beginIothub, endIothub, beginHost, endHost, beginKey;
    size_t totalLen = strlen(accountInfo->connString);

    if (sscanf(accountInfo->connString, "HostName=%n%*[^.]%n.%n%*[^;];%nSharedAccessKeyName=%n%*[^;];%nSharedAccessKey=%n", &beginHost, &endHost, &beginIothub, &endIothub, &beginName, &endName, &beginKey) != 0)
    {
        LogError("Failure determining the string length parameters.\r\n");
        result = MU_FAILURE;
    }
    else
    {
        if ((accountInfo->iothubName = (char*)malloc(endHost - beginHost + 1)) == NULL)
        {
            LogError("Failure allocating iothubName.\r\n");
            result = MU_FAILURE;
        }
        else if ((accountInfo->hostname = (char*)malloc(endIothub - beginHost + 1)) == NULL)
        {
            LogError("Failure allocating hostname.\r\n");
            free(accountInfo->iothubName);
            result = MU_FAILURE;
        }
        else if ((accountInfo->keyName = (char*)malloc(endName - beginName + 1)) == NULL)
        {
            LogError("Failure allocating hostName.\r\n");
            free(accountInfo->iothubName);
            free(accountInfo->hostname);
            result = MU_FAILURE;
        }
        else if ((accountInfo->sharedAccessKey = (char*)malloc(totalLen + 1 - beginKey + 1)) == NULL)
        {
            LogError("Failure allocating hostName.\r\n");
            free(accountInfo->iothubName);
            free(accountInfo->keyName);
            free(accountInfo->hostname);
            result = MU_FAILURE;
        }
        else if (sscanf(accountInfo->connString, "HostName=%[^.].%[^;];SharedAccessKeyName=%[^;];SharedAccessKey=%s", accountInfo->iothubName,
            accountInfo->hostname + endHost - beginHost + 1,
            accountInfo->keyName,
            accountInfo->sharedAccessKey) != 4)
        {
            LogError("Failure determining the string values.\r\n");
            free(accountInfo->iothubName);
            free(accountInfo->hostname);
            free(accountInfo->keyName);
            free(accountInfo->sharedAccessKey);
            result = MU_FAILURE;
        }
        else
        {
            (void)strcpy(accountInfo->hostname, accountInfo->iothubName);
            accountInfo->hostname[endHost - beginHost] = '.';
            if (mallocAndStrcpy_s(&accountInfo->iothubSuffix, accountInfo->hostname + endHost - beginHost + 1) != 0)
            {
                LogError("[IoTHubAccount] Failure constructing the iothubSuffix.");
                free(accountInfo->iothubName);
                free(accountInfo->hostname);
                free(accountInfo->keyName);
                free(accountInfo->sharedAccessKey);
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}

#ifdef MBED_BUILD_TIMESTAMP
static const char* getMbedParameter(const char* name)
{
    static char value[MBED_PARAM_MAX_LENGTH];
    (void)printf("%s?\r\n", name);
    (void)scanf("%s", &value);
    (void)printf("Received '%s'\r\n", value);

    return value;
}
#endif
static const char* CONN_HOST_PART = "HostName=";
static const char* CONN_DEVICE_PART = ";DeviceId=";
static const char* CONN_KEY_PART = ";SharedAccessKey=";
static const char* CONN_X509_PART = ";x509=true";
static const char* CONN_MODULE_PART = ";ModuleId=";

static int createSASConnectionString(IOTHUB_ACCOUNT_INFO* accountInfo, const char* deviceId, const char* moduleId, const char* primaryAuthentication, char** connectionString) {

    int result;
    char* conn;
    size_t sizeOfHostPart = strlen(CONN_HOST_PART);

    size_t sizeOfDevicePart = strlen(CONN_DEVICE_PART);

    size_t sizeOfKeyPart = strlen(CONN_KEY_PART);

    size_t sizeOfHostName = strlen(accountInfo->hostname);
    size_t sizeOfDeviceId = strlen(deviceId);
    size_t sizeOfDeviceKey = strlen(primaryAuthentication);

    size_t sizeOfModulePart = moduleId ? strlen(CONN_MODULE_PART) : 0;
    size_t sizeOfModuleId = moduleId ? strlen(moduleId) : 0;

    size_t connectionStringLength = sizeOfHostPart + sizeOfDevicePart + sizeOfKeyPart + sizeOfHostName + sizeOfDeviceId + sizeOfDeviceKey + sizeOfModulePart + sizeOfModuleId + 1;

    conn = (char*)malloc(connectionStringLength);
    if (conn == NULL)
    {
        LogError("Failed to allocate space for the SAS based connection string\r\n");
        result = MU_FAILURE;
    }
    else if ((moduleId == NULL) && sprintf_s(conn, connectionStringLength,"%s%s%s%s%s%s", CONN_HOST_PART, accountInfo->hostname, CONN_DEVICE_PART, (char*)deviceId, CONN_KEY_PART, (char*)primaryAuthentication) <= 0)
    {
        LogError("Failed to form the connection string for SAS based connection string.\r\n");
        free(conn);
        result = MU_FAILURE;
    }
    else if ((moduleId != NULL) && sprintf_s(conn, connectionStringLength,"%s%s%s%s%s%s%s%s", CONN_HOST_PART, accountInfo->hostname, CONN_DEVICE_PART, (char*)deviceId, CONN_KEY_PART, (char*)primaryAuthentication, CONN_MODULE_PART, moduleId) <= 0)
    {
        LogError("Failed to form the connection string for SAS based connection string.\r\n");
        free(conn);
        result = MU_FAILURE;
    }
    else
    {
        *connectionString = conn;
        result = 0;
    }
    return result;

}

static int createX509ConnectionString(IOTHUB_ACCOUNT_INFO* accountInfo, char** connectionString) {

    int result;
    char* conn;

    size_t sizeOfHostPart = strlen(CONN_HOST_PART);
    size_t sizeOfDevicePart = strlen(CONN_DEVICE_PART);
    size_t sizeOfX509Part = strlen(CONN_X509_PART);

    size_t sizeOfHostName = strlen(accountInfo->hostname);
    size_t sizeOfDeviceId = strlen(accountInfo->x509Device.deviceId);

    size_t connectionStringLength = sizeOfHostPart + sizeOfDevicePart + sizeOfX509Part + sizeOfHostName + sizeOfDeviceId + 1;

    conn = (char*)malloc(connectionStringLength);
    if (conn == NULL) {
        LogError("Failed to allocate space for the SAS based connection string\r\n");
        result = MU_FAILURE;
    }
    else if (sprintf_s(conn, connectionStringLength, "%s%s%s%s%s", CONN_HOST_PART, accountInfo->hostname, CONN_DEVICE_PART, (char*)accountInfo->x509Device.deviceId, CONN_X509_PART) <=0) {
        LogError("Failed to form the connection string for x509 based connection string.\r\n");
        result = MU_FAILURE;
        free(conn);
    }
    else
    {
        *connectionString = conn;
        result = 0;
    }
    return result;
}

static int provisionDevice(IOTHUB_ACCOUNT_INFO* accountInfo, IOTHUB_ACCOUNT_AUTH_METHOD method, IOTHUB_PROVISIONED_DEVICE* deviceToProvision)
{

    int result;
    char* deviceId = NULL;

    if (generateDeviceName(method == IOTHUB_ACCOUNT_AUTH_CONNSTRING ?(SAS_DEVICE_PREFIX_FMT):(X509_DEVICE_PREFIX_FMT), &deviceId) != 0)
    {
        LogError("generateDeviceName failed\r\n");
        result = MU_FAILURE;
    }
    else
    {

        IOTHUB_REGISTRYMANAGER_RESULT iothub_registrymanager_result;
        IOTHUB_REGISTRY_DEVICE_CREATE deviceCreateInfo;
        IOTHUB_DEVICE deviceInfo;
        deviceInfo.deviceId = NULL;
        deviceInfo.primaryKey = NULL;
        deviceInfo.secondaryKey = NULL;
        deviceInfo.generationId = NULL;
        deviceInfo.eTag = NULL;
        deviceInfo.connectionStateUpdatedTime = NULL;
        deviceInfo.statusReason = NULL;
        deviceInfo.statusUpdatedTime = NULL;
        deviceInfo.lastActivityTime = NULL;
        deviceInfo.configuration = NULL;
        deviceInfo.deviceProperties = NULL;
        deviceInfo.serviceProperties = NULL;

        deviceCreateInfo.deviceId = deviceId;
        if (method == IOTHUB_ACCOUNT_AUTH_CONNSTRING)
        {
            deviceToProvision->howToCreate = IOTHUB_ACCOUNT_AUTH_CONNSTRING;
            deviceCreateInfo.primaryKey = "";
            deviceCreateInfo.secondaryKey = "";
            deviceCreateInfo.authMethod = IOTHUB_REGISTRYMANAGER_AUTH_SPK;
        }
        else
        {
            deviceToProvision->howToCreate = IOTHUB_ACCOUNT_AUTH_X509;
            deviceCreateInfo.primaryKey = accountInfo->x509Thumbprint;
            deviceCreateInfo.secondaryKey = "";
            deviceCreateInfo.authMethod = IOTHUB_REGISTRYMANAGER_AUTH_X509_THUMBPRINT;
        }

        iothub_registrymanager_result = IoTHubRegistryManager_CreateDevice(accountInfo->iothub_registrymanager_handle, &deviceCreateInfo, &deviceInfo);
        if (iothub_registrymanager_result != IOTHUB_REGISTRYMANAGER_OK)
        {
            LogError("IoTHubRegistryManager_CreateDevice failed\r\n");
            free(deviceId);
            result = MU_FAILURE;
        }
        else
        {
            deviceToProvision->deviceId = deviceId;
            if (method == IOTHUB_ACCOUNT_AUTH_CONNSTRING)
            {
                if (mallocAndStrcpy_s((char**)&deviceToProvision->primaryAuthentication, (char*)deviceInfo.primaryKey) != 0)
                {
                    LogError("mallocAndStrcpy_s failed for primaryKey\r\n");
                    result = MU_FAILURE;

                }
                else
                {
                    if (createSASConnectionString(accountInfo, deviceToProvision->deviceId, NULL, deviceToProvision->primaryAuthentication, &deviceToProvision->connectionString) != 0)
                    {
                        result = MU_FAILURE;
                    }
                    else
                    {
                        result = 0;
                    }
                }
            }
            else
            {
                if (createX509ConnectionString(accountInfo, &deviceToProvision->connectionString) != 0)
                {
                    result = MU_FAILURE;
                }
                else
                {
                    deviceToProvision->certificate = accountInfo->x509Certificate;
                    deviceToProvision->primaryAuthentication = accountInfo->x509PrivateKey;
                    result = 0;
                }
            }
        }

        if (deviceInfo.deviceId != NULL)
            free((char*)deviceInfo.deviceId);
        if (deviceInfo.primaryKey != NULL)
            free((char*)deviceInfo.primaryKey);
        if (deviceInfo.secondaryKey != NULL)
            free((char*)deviceInfo.secondaryKey);
        if (deviceInfo.generationId != NULL)
            free((char*)deviceInfo.generationId);
        if (deviceInfo.eTag != NULL)
            free((char*)deviceInfo.eTag);
        if (deviceInfo.connectionStateUpdatedTime != NULL)
            free((char*)deviceInfo.connectionStateUpdatedTime);
        if (deviceInfo.statusReason != NULL)
            free((char*)deviceInfo.statusReason);
        if (deviceInfo.statusUpdatedTime != NULL)
            free((char*)deviceInfo.statusUpdatedTime);
        if (deviceInfo.lastActivityTime != NULL)
            free((char*)deviceInfo.lastActivityTime);
        if (deviceInfo.configuration != NULL)
            free((char*)deviceInfo.configuration);
        if (deviceInfo.deviceProperties != NULL)
            free((char*)deviceInfo.deviceProperties);
        if (deviceInfo.serviceProperties != NULL)
            free((char*)deviceInfo.serviceProperties);
    }

    return result;
}

static int provisionDevices(IOTHUB_ACCOUNT_INFO* accountInfo, IOTHUB_ACCOUNT_AUTH_METHOD method, IOTHUB_PROVISIONED_DEVICE** devicesToProvision, size_t number_of_devices)
{
    int result;
    size_t iterator;

    result = 0;

    for (iterator = 0; iterator < number_of_devices; iterator++)
    {
        if ((devicesToProvision[iterator] = malloc(sizeof(IOTHUB_PROVISIONED_DEVICE))) == NULL)
        {
            LogError("Failed creating IOTHUB_PROVISIONED_DEVICE instance for device number %lu", (unsigned long)iterator);
            result = MU_FAILURE;
            break;
        }
        else
        {
            memset(devicesToProvision[iterator], 0, sizeof(IOTHUB_PROVISIONED_DEVICE));

            if (provisionDevice(accountInfo, method, devicesToProvision[iterator]) != 0)
            {
                LogError("Failed provisioning device number %lu", (unsigned long)iterator);
                result = MU_FAILURE;
                break;
            }
        }
    }

    return result;
}

static int updateTestModule(IOTHUB_REGISTRYMANAGER_HANDLE iothub_registrymanager_handle, IOTHUB_PROVISIONED_DEVICE* deviceToProvision)
{
    int result;
    IOTHUB_REGISTRY_MODULE_UPDATE moduleUpdate;
    IOTHUB_MODULE moduleInfo;

    // Update the auth method type from its initially set NONE to IOTHUB_REGISTRYMANAGER_AUTH_SPK.
    moduleUpdate.version = IOTHUB_REGISTRY_MODULE_UPDATE_VERSION_1;
    moduleUpdate.authMethod = IOTHUB_REGISTRYMANAGER_AUTH_SPK;
    moduleUpdate.deviceId = deviceToProvision->deviceId;
    moduleUpdate.primaryKey = "";
    moduleUpdate.secondaryKey = "";
    moduleUpdate.moduleId = TEST_MODULE_NAME;
    moduleUpdate.status = IOTHUB_DEVICE_STATUS_ENABLED;
    moduleUpdate.managedBy = TEST_MANAGED_BY_2;

    memset(&moduleInfo, 0, sizeof(moduleInfo));
    moduleInfo.version = IOTHUB_MODULE_VERSION_1;


    IOTHUB_REGISTRYMANAGER_RESULT iothub_registrymanager_result = IoTHubRegistryManager_UpdateModule(iothub_registrymanager_handle, &moduleUpdate);
    if (iothub_registrymanager_result != IOTHUB_REGISTRYMANAGER_OK)
    {
        LogError("IoTHubRegistryManager_UpdateModule failed, err=%d", iothub_registrymanager_result);
        result = MU_FAILURE;
    }
    else if ((iothub_registrymanager_result = IoTHubRegistryManager_GetModule(iothub_registrymanager_handle, deviceToProvision->deviceId, deviceToProvision->moduleId, &moduleInfo)) != IOTHUB_REGISTRYMANAGER_OK)
    {
        LogError("IoTHubRegistryManager_UpdateModule(deviceId=%s, moduleId=%s) failed, err=%d", deviceToProvision->deviceId, deviceToProvision->moduleId, iothub_registrymanager_result);
        result = MU_FAILURE;
    }
    else if (strcmp(moduleInfo.moduleId, TEST_MODULE_NAME) != 0)
    {
        LogError("ModuleName expected (%s) does not match what was returned from IoTHubRegistryManager_CreateModule (%s)", TEST_MODULE_NAME, moduleInfo.moduleId);
        result = MU_FAILURE;
    }
    else if (strcmp(deviceToProvision->deviceId, moduleInfo.deviceId) != 0)
    {
        LogError("DeviceId expected (%s) does not match what was returned from IoTHubRegistryManager_CreateModule (%s)", deviceToProvision->deviceId, moduleInfo.deviceId);
        result = MU_FAILURE;
    }
#if 0  // IoTHub currently returns incorrect value.  Will bring back in once fixed.
    else if (strcmp(moduleInfo.managedBy, TEST_MANAGED_BY_2) != 0)
    {
        LogError("IoTHubRegistryManager_UpdateModule sets managedBy=%s, expected=%s", moduleInfo.managedBy, TEST_MANAGED_BY_2);
        result = MU_FAILURE;
    }
#endif
    else
    {
        result = 0;
    }

    IoTHubRegistryManager_FreeModuleMembers(&moduleInfo);
    return result;
}

static int provisionModule(IOTHUB_ACCOUNT_INFO* accountInfo, IOTHUB_PROVISIONED_DEVICE* deviceToProvision)
{
    IOTHUB_REGISTRY_MODULE_CREATE moduleCreate;
    IOTHUB_MODULE moduleInfo;
    int result;

    // We set the initial auth type to none, to simulate and test more closely how Edge
    // modules are created.  We'll upgrade this to SPK after creation.
    moduleCreate.version = IOTHUB_REGISTRY_MODULE_CREATE_VERSION_1;
    moduleCreate.authMethod = IOTHUB_REGISTRYMANAGER_AUTH_NONE;
    moduleCreate.deviceId = deviceToProvision->deviceId;
    moduleCreate.moduleId = TEST_MODULE_NAME;
    moduleCreate.primaryKey = "";
    moduleCreate.secondaryKey = "";
    moduleCreate.managedBy = TEST_MANAGED_BY_1;

    // Even though we already have a IOTHUB_SERVICE_CLIENT_AUTH_HANDLE handle (iothub_account_info->iothub_service_client_auth_handle), we get a
    // new one based on the device's connection string (not the Hub) as we need to test this scenario, too.
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE service_auth_from_device_connection = NULL;
    IOTHUB_REGISTRYMANAGER_RESULT iothub_registrymanager_result;
    IOTHUB_REGISTRYMANAGER_HANDLE iothub_registrymanager_handle = NULL;

    memset(&moduleInfo, 0, sizeof(moduleInfo));
    moduleInfo.version = IOTHUB_MODULE_VERSION_1;

    if (mallocAndStrcpy_s(&deviceToProvision->moduleId, TEST_MODULE_NAME) != 0)
    {
        LogError("mallocAndStrcpy_s failed");
        result = MU_FAILURE;
    }
    else if ((service_auth_from_device_connection = IoTHubServiceClientAuth_CreateFromConnectionString(deviceToProvision->connectionString)) == NULL)
    {
        LogError("IoTHubServiceClientAuth_CreateFromConnectionString(%s) failed", deviceToProvision->connectionString);
        result = MU_FAILURE;
    }
    else if ((iothub_registrymanager_handle = IoTHubRegistryManager_Create(service_auth_from_device_connection)) == NULL)
    {
        LogError("IoTHubServiceClientAuth_CreateFromConnectionString(%s) failed", deviceToProvision->connectionString);
        result = MU_FAILURE;
    }
    else if ((iothub_registrymanager_result = IoTHubRegistryManager_CreateModule(iothub_registrymanager_handle, &moduleCreate, &moduleInfo)) != IOTHUB_REGISTRYMANAGER_OK)
    {
        LogError("IoTHubRegistryManager_CreateModule failed, err=%d", iothub_registrymanager_result);
        result = MU_FAILURE;
    }
    else if (strcmp(moduleInfo.moduleId, TEST_MODULE_NAME) != 0)
    {
        LogError("ModuleName expected (%s) does not match what was returned from IoTHubRegistryManager_CreateModule (%s)", TEST_MODULE_NAME, moduleInfo.moduleId);
        result = MU_FAILURE;
    }
    else if (strcmp(deviceToProvision->deviceId, moduleInfo.deviceId) != 0)
    {
        LogError("DeviceId expected (%s) does not match what was returned from IoTHubRegistryManager_CreateModule (%s)", deviceToProvision->deviceId, moduleInfo.deviceId);
        result = MU_FAILURE;
    }
#if 0 // IoTHub currently returns incorrect value.  Will bring back in once fixed.
    else if (strcmp(TEST_MANAGED_BY_1, moduleInfo.managedBy) != 0)
    {
        LogError("managedBy expected (%s) does not match what was returned from IoTHubRegistryManager_CreateModule (%s)", TEST_MANAGED_BY_1, moduleInfo.managedBy);
        result = MU_FAILURE;
    }
#endif
    else if (updateTestModule(iothub_registrymanager_handle, deviceToProvision) != 0)
    {
        LogError("Unable to update test module");
        result = MU_FAILURE;
    }
    else if (createSASConnectionString(accountInfo, deviceToProvision->deviceId, TEST_MODULE_NAME, deviceToProvision->primaryAuthentication, &deviceToProvision->moduleConnectionString) != 0)
    {
        LogError("createSASConnectionStringForModule failed");
        result = MU_FAILURE;
    }
    else
    {
        LogInfo("Created device/module = %s/%s", deviceToProvision->deviceId, TEST_MODULE_NAME);
        result = 0;
    }

    IoTHubRegistryManager_FreeModuleMembers(&moduleInfo);

    if (iothub_registrymanager_handle != NULL)
    {
        IoTHubRegistryManager_Destroy(iothub_registrymanager_handle);
    }
    if (service_auth_from_device_connection != NULL)
    {
        IoTHubServiceClientAuth_Destroy(service_auth_from_device_connection);
    }

    return result;
}

static char* convert_base64_to_string(const char* base64_cert)
{
    char* result;
    BUFFER_HANDLE raw_cert = Azure_Base64_Decode(base64_cert);
    if (raw_cert == NULL)
    {
        LogError("Failure decoding base64 encoded cert.\r\n");
        result = NULL;
    }
    else
    {
        STRING_HANDLE cert = STRING_from_byte_array(BUFFER_u_char(raw_cert), BUFFER_length(raw_cert));
        if (cert == NULL)
        {
            LogError("Failure creating cert from binary.\r\n");
            result = NULL;
        }
        else
        {
            if (mallocAndStrcpy_s(&result, STRING_c_str(cert)) != 0)
            {
                LogError("Failure allocating certificate.\r\n");
                result = NULL;
            }
            STRING_delete(cert);
        }
        BUFFER_delete(raw_cert);
    }
    return result;
}

IOTHUB_ACCOUNT_INFO_HANDLE IoTHubAccount_Init_With_Config(IOTHUB_ACCOUNT_CONFIG* config, bool testingModules)
{
    IOTHUB_ACCOUNT_INFO* iothub_account_info;

    if (config == NULL)
    {
        LogError("[IoTHubAccount] invalid configuration (NULL)");
        iothub_account_info = NULL;
    }
    else
    {
        iothub_account_info = malloc(sizeof(IOTHUB_ACCOUNT_INFO));

        if (iothub_account_info == NULL)
        {
            LogError("[IoTHubAccount] Failed allocating IOTHUB_ACCOUNT_INFO.");
        }
        else
        {
            char* base64_cert;
            char* base64_key;
            char* tempThumb;
            memset(iothub_account_info, 0, sizeof(IOTHUB_ACCOUNT_INFO));

            if ((iothub_account_info->sasDevices = (IOTHUB_PROVISIONED_DEVICE**)malloc(sizeof(IOTHUB_PROVISIONED_DEVICE*) * config->number_of_sas_devices)) == NULL)
            {
                LogError("[IoTHubAccount] Failed allocating array for SAS devices");
                free(iothub_account_info);
                iothub_account_info = NULL;
            }
            else
            {
                iothub_account_info->number_of_sas_devices = config->number_of_sas_devices;
                memset(iothub_account_info->sasDevices, 0, sizeof(IOTHUB_PROVISIONED_DEVICE*) * iothub_account_info->number_of_sas_devices);

#ifdef MBED_BUILD_TIMESTAMP
                iothub_account_info->connString = getMbedParameter("IOTHUB_CONNECTION_STRING");
                iothub_account_info->eventhubConnString = getMbedParameter("IOTHUB_EVENTHUB_CONNECTION_STRING");
                tempCert = getMbedParameter("IOTHUB_E2E_X509_CERT");
                tempKey = getMbedParameter("IOTHUB_E2E_X509_PRIVATE_KEY");
                tempThump = getMbedParameter("IOTHUB_E2E_X509_THUMBPRINT");
#else
                iothub_account_info->connString = getenv("IOTHUB_CONNECTION_STRING");
                iothub_account_info->eventhubConnString = getenv("IOTHUB_EVENTHUB_CONNECTION_STRING");
                base64_cert = getenv("IOTHUB_E2E_X509_CERT_BASE64");
                base64_key = getenv("IOTHUB_E2E_X509_PRIVATE_KEY_BASE64");
                tempThumb = getenv("IOTHUB_E2E_X509_THUMBPRINT");
#endif

                if (iothub_account_info->connString == NULL)
                {
                    LogError("Failure retrieving IoT Hub connection string from the environment.\r\n");
                    free(iothub_account_info);
                    iothub_account_info = NULL;
                }
                else if (iothub_account_info->eventhubConnString == NULL)
                {
                    LogError("Failure retrieving Event Hub connection string from the environment.\r\n");
                    free(iothub_account_info);
                    iothub_account_info = NULL;
                }
                else if (base64_cert == NULL)
                {
                    LogError("Failure retrieving x509 certificate from the environment.\r\n");
                    free(iothub_account_info);
                    iothub_account_info = NULL;
                }
                else if (base64_key == NULL)
                {
                    LogError("Failure retrieving x509 private key from the environment.\r\n");
                    free(iothub_account_info);
                    iothub_account_info = NULL;
                }
                else if (tempThumb == NULL)
                {
                    LogError("Failure retrieving x509 certificate thumbprint from the environment.\r\n");
                    free(iothub_account_info);
                    iothub_account_info = NULL;
                }
                else if ((iothub_account_info->x509Certificate = convert_base64_to_string(base64_cert)) == NULL)
                {
                    LogError("Failure allocating x509 certificate from the environment.\r\n");
                    free(iothub_account_info);
                    iothub_account_info = NULL;
                }
                else if ((iothub_account_info->x509PrivateKey = convert_base64_to_string(base64_key)) == NULL)
                {
                    LogError("Failure allocating x509 key from the environment.\r\n");
                    free(iothub_account_info->x509Certificate);
                    free(iothub_account_info);
                    iothub_account_info = NULL;
                }
                else if (mallocAndStrcpy_s(&iothub_account_info->x509Thumbprint, tempThumb) != 0)
                {
                    LogError("Failure allocating x509 thumb print from the environment.\r\n");
                    free(iothub_account_info->x509Certificate);
                    free(iothub_account_info->x509PrivateKey);
                    free(iothub_account_info);
                    iothub_account_info = NULL;
                }
                else
                {
                    if (retrieveConnStringInfo(iothub_account_info) != 0)
                    {
                        LogError("retrieveConnStringInfo failed.\r\n");
                        free(iothub_account_info);
                        iothub_account_info = NULL;
                    }
                    else
                    {
                        iothub_account_info->iothub_service_client_auth_handle = IoTHubServiceClientAuth_CreateFromConnectionString(iothub_account_info->connString);
                        if (iothub_account_info->iothub_service_client_auth_handle == NULL)
                        {
                            LogError("IoTHubServiceClientAuth_CreateFromConnectionString failed\r\n");
                            free(iothub_account_info);
                            iothub_account_info = NULL;
                        }
                        else
                        {
                            iothub_account_info->iothub_messaging_handle = IoTHubMessaging_LL_Create(iothub_account_info->iothub_service_client_auth_handle);
                            if (iothub_account_info->iothub_messaging_handle == NULL)
                            {
                                LogError("IoTHubMessaging_LL_Create failed\r\n");
                                IoTHubAccount_deinit(iothub_account_info);
                                iothub_account_info = NULL;
                            }
                            else
                            {
                                iothub_account_info->iothub_registrymanager_handle = IoTHubRegistryManager_Create(iothub_account_info->iothub_service_client_auth_handle);
                                if (iothub_account_info->iothub_registrymanager_handle == NULL)
                                {
                                    LogError("IoTHubRegistryManager_Create failed\r\n");
                                    IoTHubAccount_deinit(iothub_account_info);
                                    iothub_account_info = NULL;
                                }
                                else
                                {
                                    if (provisionDevices(iothub_account_info, IOTHUB_ACCOUNT_AUTH_CONNSTRING, iothub_account_info->sasDevices, iothub_account_info->number_of_sas_devices) != 0)
                                    {
                                        LogError("Failed to create the SAS device(s)\r\n");
                                        IoTHubAccount_deinit(iothub_account_info);
                                        iothub_account_info = NULL;
                                    }
                                    else if (provisionDevice(iothub_account_info, IOTHUB_ACCOUNT_AUTH_X509, &iothub_account_info->x509Device) != 0)
                                    {
                                        LogError("Failed to create the X509 device\r\n");
                                        IoTHubAccount_deinit(iothub_account_info);
                                        iothub_account_info = NULL;
                                    }
                                    else if (testingModules && provisionModule(iothub_account_info, iothub_account_info->sasDevices[0]))
                                    {
                                        LogError("Failed to create the module\r\n");
                                        IoTHubAccount_deinit(iothub_account_info);
                                        iothub_account_info = NULL;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return (IOTHUB_ACCOUNT_INFO_HANDLE)iothub_account_info;
}

IOTHUB_ACCOUNT_INFO_HANDLE IoTHubAccount_Init(bool testingModules)
{
    IOTHUB_ACCOUNT_CONFIG config;
    config.number_of_sas_devices = 1;

    return IoTHubAccount_Init_With_Config(&config, testingModules);
}

void IoTHubAccount_deinit(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    if (acctHandle != NULL)
    {
        IOTHUB_ACCOUNT_INFO* acctInfo = (IOTHUB_ACCOUNT_INFO*)acctHandle;
        size_t iterator;
        IOTHUB_REGISTRYMANAGER_RESULT iothub_registrymanager_result;

        for (iterator = 0; iterator < acctInfo->number_of_sas_devices; iterator++)
        {
            IOTHUB_PROVISIONED_DEVICE* provisioned_device = acctInfo->sasDevices[iterator];

            if (provisioned_device != NULL)
            {
                if (provisioned_device->deviceId != NULL)
                {
                    iothub_registrymanager_result = IoTHubRegistryManager_DeleteDevice(acctInfo->iothub_registrymanager_handle, provisioned_device->deviceId);
                    if (iothub_registrymanager_result != IOTHUB_REGISTRYMANAGER_OK)
                    {
                        LogError("IoTHubRegistryManager_DeleteDevice failed for SAS Based Device \"%s\"\r\n", provisioned_device->deviceId);
                    }
                }

                free(provisioned_device->deviceId);
                free(provisioned_device->moduleId);
                free(provisioned_device->primaryAuthentication);
                free(provisioned_device->connectionString);
                free(provisioned_device->moduleConnectionString);
                free(provisioned_device);
            }
        }

        free(acctInfo->sasDevices);

        if (acctInfo->x509Device.deviceId)
        {
            iothub_registrymanager_result = IoTHubRegistryManager_DeleteDevice(acctInfo->iothub_registrymanager_handle, acctInfo->x509Device.deviceId);
            if (iothub_registrymanager_result != IOTHUB_REGISTRYMANAGER_OK)
            {
                LogError("IoTHubRegistryManager_DeleteDevice failed for x509 Based Device\r\n");
            }
        }

        IoTHubMessaging_LL_Destroy(acctInfo->iothub_messaging_handle);
        IoTHubRegistryManager_Destroy(acctInfo->iothub_registrymanager_handle);
        IoTHubServiceClientAuth_Destroy(acctInfo->iothub_service_client_auth_handle);

        free(acctInfo->hostname);
        free(acctInfo->iothubName);
        free(acctInfo->iothubSuffix);
        free(acctInfo->sharedAccessKey);
        free(acctInfo->sharedAccessToken);
        free(acctInfo->keyName);
        free(acctInfo->eventhubAccessKey);

        free(acctInfo->x509Device.deviceId);
        free(acctInfo->x509Device.connectionString);
        free(acctInfo->x509Certificate);
        free(acctInfo->x509PrivateKey);
        free(acctInfo->x509Thumbprint);
        free(acctInfo);
    }
}

const char* IoTHubAccount_GetEventHubConnectionString(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    const char* result = NULL;
    IOTHUB_ACCOUNT_INFO* acctInfo = (IOTHUB_ACCOUNT_INFO*)acctHandle;
    if (acctInfo != NULL)
    {
        result = acctInfo->eventhubConnString;
    }
    return result;
}

const char* IoTHubAccount_GetIoTHubName(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    const char* result = NULL;
    IOTHUB_ACCOUNT_INFO* acctInfo = (IOTHUB_ACCOUNT_INFO*)acctHandle;
    if (acctInfo != NULL)
    {
        result = acctInfo->iothubName;
    }
    return result;
}

const char* IoTHubAccount_GetIoTHubSuffix(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    const char* result = NULL;
    IOTHUB_ACCOUNT_INFO* acctInfo = (IOTHUB_ACCOUNT_INFO*)acctHandle;
    if (acctInfo != NULL)
    {
        result = acctInfo->iothubSuffix;
    }
    return result;
}

const char* IoTHubAccount_GetEventhubListenName(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    static char listenName[64];
    const char* value;

#ifndef MBED_BUILD_TIMESTAMP
    if ((value = getenv("IOTHUB_EVENTHUB_LISTEN_NAME")) == NULL)
#else
    if ((value = getMbedParameter("IOTHUB_EVENTHUB_LISTEN_NAME")) == NULL)
#endif
    {
        value = IoTHubAccount_GetIoTHubName(acctHandle);
    }

    if (value != NULL &&
        sprintf_s(listenName, 64, "%s", value) <= 0)
    {
        LogError("Failed reading IoT Hub Event Hub listen namespace (sprintf_s failed).");
    }

    return listenName;
}


IOTHUB_PROVISIONED_DEVICE* IoTHubAccount_GetSASDevice(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    if (acctHandle != NULL)
    {
        return acctHandle->sasDevices[0];
    }
    else
    {
        return NULL;
    }
}

IOTHUB_PROVISIONED_DEVICE** IoTHubAccount_GetSASDevices(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    if (acctHandle != NULL)
    {
        return acctHandle->sasDevices;
    }
    else
    {
        return NULL;
    }
}

IOTHUB_PROVISIONED_DEVICE* IoTHubAccount_GetX509Device(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    if (acctHandle != NULL)
    {
        return &acctHandle->x509Device;
    }
    else
    {
        return NULL;
    }
}

const char* IoTHubAccount_GetIoTHubConnString(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    if (acctHandle != NULL)
    {
        return ((IOTHUB_ACCOUNT_INFO*)acctHandle)->connString;
    }
    else
    {
        return NULL;
    }
}

const char* IoTHubAccount_GetSharedAccessSignature(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    const char* result = NULL;
    IOTHUB_ACCOUNT_INFO* acctInfo = (IOTHUB_ACCOUNT_INFO*)acctHandle;
    if (acctInfo != NULL)
    {
        if (acctInfo->sharedAccessToken != NULL)
        {
            // Reuse the sharedAccessToken if it's been created already
            result = acctInfo->sharedAccessToken;
        }
        else
        {
        time_t currentTime = time(NULL);
        size_t expiry_time = (size_t)(currentTime + 3600);

        STRING_HANDLE accessKey = STRING_construct(acctInfo->sharedAccessKey);
        STRING_HANDLE iotName = STRING_construct(acctInfo->hostname);
        STRING_HANDLE keyName = STRING_construct(acctInfo->keyName);
        if (accessKey != NULL && iotName != NULL && keyName != NULL)
        {
            STRING_HANDLE sasHandle = SASToken_Create(accessKey, iotName, keyName, expiry_time);
            if (sasHandle == NULL)
            {

                result = NULL;
            }
            else
            {
                if (mallocAndStrcpy_s(&acctInfo->sharedAccessToken, STRING_c_str(sasHandle)) != 0)
                {
                    result = NULL;
                }
                else
                {
                    result = acctInfo->sharedAccessToken;
                }
                STRING_delete(sasHandle);
            }
        }
        STRING_delete(accessKey);
        STRING_delete(iotName);
        STRING_delete(keyName);
    }
    }
    else
    {
        result = NULL;
    }
    return result;
}

const char* IoTHubAccount_GetEventhubAccessKey(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    const char *iothub_connection_string;
    static char access_key[128];

    if ((iothub_connection_string = IoTHubAccount_GetIoTHubConnString(acctHandle)) != NULL)
    {
        STRING_HANDLE iothub_connection_string_str;

        if ((iothub_connection_string_str = STRING_construct(iothub_connection_string)) != NULL)
        {
            STRING_TOKENIZER_HANDLE tokenizer;

            if ((tokenizer = STRING_TOKENIZER_create(iothub_connection_string_str)) != NULL)
            {
                STRING_HANDLE tokenString;

                if ((tokenString = STRING_new()) != NULL)
                {
                    STRING_HANDLE valueString;

                    if ((valueString = STRING_new()) != NULL)
                    {
                        while ((STRING_TOKENIZER_get_next_token(tokenizer, tokenString, "=") == 0))
                        {
                            char tokenValue[128];
                            strcpy(tokenValue, STRING_c_str(tokenString));

                            if (STRING_TOKENIZER_get_next_token(tokenizer, tokenString, ";") != 0)
                            {
                                break;
                            }

                            if (strcmp(tokenValue, "SharedAccessKey") == 0)
                            {
                                strcpy(access_key, STRING_c_str(tokenString));
                                break;
                            }
                        }

                        STRING_delete(valueString);
                    }

                    STRING_delete(tokenString);
                }

                STRING_TOKENIZER_destroy(tokenizer);
            }

            STRING_delete(iothub_connection_string_str);
        }
    }

    return access_key;
}

const char* IoTHubAccount_GetEventhubConsumerGroup(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    (void)acctHandle;
#ifdef MBED_BUILD_TIMESTAMP
    return getMbedParameter("IOTHUB_EVENTHUB_CONSUMER_GROUP");
#else
    static char consumerGroup[64];
    char *envVarValue = getenv("IOTHUB_EVENTHUB_CONSUMER_GROUP");
    if (envVarValue != NULL)
    {
        strcpy(consumerGroup, envVarValue);
    }
    else
    {
        strcpy(consumerGroup, DEFAULT_CONSUMER_GROUP);
    }
    return consumerGroup;
#endif
}

const size_t IoTHubAccount_GetIoTHubPartitionCount(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    int value;
    (void)acctHandle;
#ifdef MBED_BUILD_TIMESTAMP
    char* str_value;
    if ((str_value = getMbedParameter("IOTHUB_PARTITION_COUNT")) != NULL)
    {
        sscanf(str_value, "%i", &value);
    }
#else
    char *envVarValue = getenv("IOTHUB_PARTITION_COUNT");
    if (envVarValue != NULL)
    {
        value = atoi(envVarValue);
    }
    else
    {
        value = DEFAULT_PARTITION_COUNT;
    }
#endif
    return (size_t)value;
}

const IOTHUB_MESSAGING_HANDLE IoTHubAccount_GetMessagingHandle(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle)
{
    IOTHUB_MESSAGING_HANDLE result = NULL;
    IOTHUB_ACCOUNT_INFO* acctInfo = (IOTHUB_ACCOUNT_INFO*)acctHandle;
    if (acctInfo != NULL)
    {
        result = acctInfo->iothub_messaging_handle;
    }
    return result;
}

