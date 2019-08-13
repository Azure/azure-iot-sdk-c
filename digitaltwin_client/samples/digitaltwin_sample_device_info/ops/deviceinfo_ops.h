// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DEVICEINFO_OPS_H
#define DEVICEINFO_OPS_H

#ifndef RESULT_OK
#define RESULT_OK 0
#endif
#ifndef RESULT_FAILURE
#define RESULT_FAILURE -1
#endif

#ifdef __cplusplus
extern "C"
{
#endif

int DeviceInformation_SwVersionProperty(char** returnValue);
int DeviceInformation_ManufacturerProperty(char** returnValue);
int DeviceInformation_ModelProperty(char** returnValue);
int DeviceInformation_OsNameProperty(char** returnValue);
int DeviceInformation_ProcessorArchitectureProperty(char** returnValue);
int DeviceInformation_ProcessorManufacturerProperty(char** returnValue);
int DeviceInformation_TotalStorageProperty(char** returnValue);
int DeviceInformation_TotalMemoryProperty(char** returnValue);

int DI_serializeCharData(const char* returnValue, char** result);
int DI_serializeIntegerData(const int returnValue, char** result);
int DI_serializeCharDataFromFile(const char* sourcePath, char** returnValue);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DEVICEINFO_OPS_H