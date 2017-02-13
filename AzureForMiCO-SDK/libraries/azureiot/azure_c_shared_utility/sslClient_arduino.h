// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SSLCLIENT_ARDUINO_H
#define SSLCLIENT_ARDUINO_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

#include "umock_c_prod.h"

MOCKABLE_FUNCTION(, void, sslClient_setTimeout, unsigned long, timeout);
MOCKABLE_FUNCTION(, uint8_t, sslClient_connected);
MOCKABLE_FUNCTION(, int, sslClient_connect, uint32_t, ipAddress, uint16_t, port);
MOCKABLE_FUNCTION(, void, sslClient_stop);
MOCKABLE_FUNCTION(, size_t, sslClient_write, const uint8_t*, buf, size_t, size);
MOCKABLE_FUNCTION(, size_t, sslClient_print, const char*, str);
MOCKABLE_FUNCTION(, int, sslClient_read, uint8_t*, buf, size_t, size);
MOCKABLE_FUNCTION(, int, sslClient_available);

MOCKABLE_FUNCTION(, uint8_t, sslClient_hostByName, const char*, hostName, uint32_t*, ipAddress);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SSLCLIENT_ARDUINO_H */

