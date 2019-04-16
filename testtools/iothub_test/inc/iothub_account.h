// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBACCOUNT_H
#define IOTHUBACCOUNT_H

#include "iothub_messaging_ll.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

#define IOTHUB_ACCOUNT_AUTH_METHOD_VALUES      \
    IOTHUB_ACCOUNT_AUTH_CONNSTRING,            \
    IOTHUB_ACCOUNT_AUTH_X509                   \

MU_DEFINE_ENUM(IOTHUB_ACCOUNT_AUTH_METHOD, IOTHUB_ACCOUNT_AUTH_METHOD_VALUES);

typedef struct IOTHUB_PROVISIONED_DEVICE_TAG {
    char* connectionString;
    char* primaryAuthentication;
    char* certificate;
    char* deviceId;
    char* moduleId;
    char* moduleConnectionString;
    IOTHUB_ACCOUNT_AUTH_METHOD howToCreate;
} IOTHUB_PROVISIONED_DEVICE;

typedef struct IOTHUB_ACCOUNT_CONFIG_TAG
{
    size_t number_of_sas_devices;
} IOTHUB_ACCOUNT_CONFIG;

typedef struct IOTHUB_ACCOUNT_INFO_TAG* IOTHUB_ACCOUNT_INFO_HANDLE;

extern IOTHUB_ACCOUNT_INFO_HANDLE IoTHubAccount_Init(bool testingModules);
extern IOTHUB_ACCOUNT_INFO_HANDLE IoTHubAccount_Init_With_Config(IOTHUB_ACCOUNT_CONFIG* config, bool testingModules);
extern void IoTHubAccount_deinit(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);

extern const char* IoTHubAccount_GetEventHubConnectionString(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern const char* IoTHubAccount_GetIoTHubName(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern const char* IoTHubAccount_GetIoTHubSuffix(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern IOTHUB_PROVISIONED_DEVICE* IoTHubAccount_GetSASDevice(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern IOTHUB_PROVISIONED_DEVICE** IoTHubAccount_GetSASDevices(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern IOTHUB_PROVISIONED_DEVICE* IoTHubAccount_GetX509Device(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern const char* IoTHubAccount_GetEventhubListenName(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern const char* IoTHubAccount_GetIoTHubConnString(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern const char* IoTHubAccount_GetSharedAccessSignature(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern const char* IoTHubAccount_GetEventhubAccessKey(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern const size_t IoTHubAccount_GetIoTHubPartitionCount(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern const char* IoTHubAccount_GetEventhubConsumerGroup(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);
extern const IOTHUB_MESSAGING_HANDLE IoTHubAccount_GetMessagingHandle(IOTHUB_ACCOUNT_INFO_HANDLE acctHandle);

#ifdef __cplusplus
}
#endif

#endif // IOTHUBACCOUNT_H
