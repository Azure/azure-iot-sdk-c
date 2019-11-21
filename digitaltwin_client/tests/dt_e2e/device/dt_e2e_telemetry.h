// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DT_E2E_TELEMETRY_H
#define DT_E2E_TELEMETRY_H

#include "digitaltwin_interface_client.h"

#ifdef __cplusplus
extern "C"
{
#endif

DIGITALTWIN_INTERFACE_CLIENT_HANDLE DT_E2E_Telemetry_CreateInterface(void);
void DT_E2E_Telemetry_DestroyInterface(DIGITALTWIN_INTERFACE_CLIENT_HANDLE interfaceHandle);

#ifdef __cplusplus
}
#endif

#endif /* DT_E2E_TELEMETRY_H */
