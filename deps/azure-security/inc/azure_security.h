// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef AZURE_SECURITY_H
#define AZURE_SECURITY_H

#include "iothub_message.h"

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#include <stdbool.h>
#endif /* __cplusplus */

IOTHUB_MESSAGE_HANDLE AzureSecurity_GetSecurityInfoMessage();


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AZURE_SECURITY_H */
