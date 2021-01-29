// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_CLIENT_COMMON_LONGHAUL_H
#define IOTHUB_CLIENT_COMMON_LONGHAUL_H

#include <stdlib.h>
#include <stddef.h>
#include "iothub_device_client.h"
#include "iothub_account.h"
#include "../common_longhaul/iothub_client_statistics.h"

typedef struct IOTHUB_LONGHAUL_RESOURCES_TAG* IOTHUB_LONGHAUL_RESOURCES_HANDLE;

extern IOTHUB_LONGHAUL_RESOURCES_HANDLE longhaul_tests_init();
extern void longhaul_tests_deinit(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle);

extern IOTHUB_ACCOUNT_INFO_HANDLE longhaul_get_account_info(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle);
extern IOTHUB_DEVICE_CLIENT_HANDLE longhaul_get_iothub_client_handle(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle);
extern IOTHUB_CLIENT_STATISTICS_HANDLE longhaul_get_statistics(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle);

extern IOTHUB_DEVICE_CLIENT_HANDLE longhaul_initialize_device_client(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, IOTHUB_PROVISIONED_DEVICE* deviceToUse, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);

extern int longhaul_start_listening_for_telemetry_messages(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle, IOTHUB_PROVISIONED_DEVICE* deviceToUse);
extern int longhaul_stop_listening_for_telemetry_messages(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle);

extern int longhaul_run_telemetry_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle);
extern int longhaul_run_c2d_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle);
extern int longhaul_run_device_methods_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle);
extern int longhaul_run_twin_desired_properties_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle);
extern int longhaul_run_twin_reported_properties_tests(IOTHUB_LONGHAUL_RESOURCES_HANDLE handle);

#endif // IOTHUB_CLIENT_COMMON_LONGHAUL_H