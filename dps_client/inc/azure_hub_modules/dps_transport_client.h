// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DPS_TRANSPORT_CLIENT_H
#define DPS_TRANSPORT_CLIENT_H

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif /* __cplusplus */

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/macro_utils.h"

typedef struct DPS_TRANSPORT_INFO_TAG* DPS_TRANSPORT_HANDLE;

#define DPS_TRANSPORT_STATUS_VALUES     \
    DPS_TRANSPORT_STATUS_ERROR,     \
    DPS_TRANSPORT_STATUS_SUCCESS,     \
    DPS_TRANSPORT_STATUS_ASSIGNING,     \
    DPS_TRANSPORT_STATUS_ASSIGNED

DEFINE_ENUM(DPS_TRANSPORT_STATUS, DPS_TRANSPORT_STATUS_VALUES);

#define DPS_ERROR_VALUES            \
    DPS_ERROR_SUCCESS,              \
    DPS_ERROR_INVALID_ARG,          \

DEFINE_ENUM(DPS_ERROR, DPS_ERROR_VALUES);

typedef void(*DPS_REGISTER_DATA_CALLBACK)(DPS_TRANSPORT_STATUS transport_status, const char* data, void* user_ctx);

MOCKABLE_FUNCTION(, DPS_TRANSPORT_HANDLE, dps_transport_create, const char* uri, DPS_REGISTER_DATA_CALLBACK*, data_callback, void*, user_ctx);
MOCKABLE_FUNCTION(, void, dps_transport_destroy, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, dps_transport_register_device, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, int, dps_transport_get_operation_status, DPS_TRANSPORT_HANDLE, handle);
MOCKABLE_FUNCTION(, void, dps_transport_dowork, DPS_TRANSPORT_HANDLE, handle);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // DPS_TRANSPORT_CLIENT_H
