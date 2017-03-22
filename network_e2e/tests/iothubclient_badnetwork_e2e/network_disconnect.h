// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef _NETWORK_DISCONNECT_H_
#define _NETWORK_DISCONNECT_H_

#include "iothub_client_ll.h"
#include "iothub_account.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern int network_disconnect();
    extern int network_reconnect();
    extern bool is_network_connected();


#ifdef __cplusplus
}
#endif

#endif // _NETWORK_DISCONNECT_H_