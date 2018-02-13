// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_CLIENT_STATISTICS_H
#define IOTHUB_CLIENT_STATISTICS_H

#include <stdlib.h>
#include "iothub_client_ll.h"

typedef enum EVENT_TYPE_TAG
{
    TELEMETRY_QUEUED, // message_id, time_queued, api_result
    TELEMETRY_SENT, // message_id, time_sent, api_result
    TELEMETRY_RECEIVED, // message_id, time_received
} EVENT_TYPE;

typedef struct TELEMETRY_INFO_TAG
{
    size_t message_id;

    time_t time_queued;
    size_t send_result;
    
    time_t time_sent;
    IOTHUB_CLIENT_CONFIRMATION_RESULT send_callback_result;
    
    time_t time_received;
} TELEMETRY_INFO;

typedef struct IOTHUB_CLIENT_STATISTICS_TAG* IOTHUB_CLIENT_STATISTICS_HANDLE;

extern IOTHUB_CLIENT_STATISTICS_HANDLE iothub_client_statistics_create(void);

extern char* iothub_client_statistics_to_json(IOTHUB_CLIENT_STATISTICS_HANDLE handle);

extern int iothub_client_statistics_add_connection_status(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);

extern int iothub_client_statistics_add_telemetry_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, EVENT_TYPE type, TELEMETRY_INFO* info);

extern int iothub_client_statistics_get_telemetry_summary(
    IOTHUB_CLIENT_STATISTICS_HANDLE handle,
    size_t* messages_sent, size_t* messages_received, double* min_travel_time, double* max_travel_time);

extern void iothub_client_statistics_destroy(IOTHUB_CLIENT_STATISTICS_HANDLE handle);

#endif // IOTHUB_CLIENT_STATISTICS_H
