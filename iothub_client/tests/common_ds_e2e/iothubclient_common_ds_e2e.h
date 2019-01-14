// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUBCLIENT_COMMON_DS_E2E_H
#define IOTHUBCLIENT_COMMON_DS_E2E_H

#include "iothub_client_ll.h"
#include "iothub_account.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif // __cplusplus

extern void ds_e2e_init(bool testing_modules);
extern void ds_e2e_deinit(void);

extern void ds_e2e_receive_device_streaming_request(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, bool shouldAcceptRequest);
extern void ds_e2e_receive_module_streaming_request(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol, bool shouldAcceptRequest);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* IOTHUBCLIENT_COMMON_DS_E2E_H */
