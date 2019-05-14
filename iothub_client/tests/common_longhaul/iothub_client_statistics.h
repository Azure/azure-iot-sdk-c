// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_CLIENT_STATISTICS_H
#define IOTHUB_CLIENT_STATISTICS_H

#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include "iothub_messaging_ll.h"
#include "iothub_devicemethod.h"
#include "iothub_devicetwin.h"
#include "iothub_client_ll.h"

#define TELEMETRY_EVENT_TYPE_VALUES \
    TELEMETRY_QUEUED, \
    TELEMETRY_SENT, \
    TELEMETRY_RECEIVED

MU_DEFINE_ENUM(TELEMETRY_EVENT_TYPE, TELEMETRY_EVENT_TYPE_VALUES)

#define C2D_EVENT_TYPE_VALUES \
    C2D_QUEUED, \
    C2D_SENT, \
    C2D_RECEIVED

MU_DEFINE_ENUM(C2D_EVENT_TYPE, C2D_EVENT_TYPE_VALUES)

#define DEVICE_METHOD_EVENT_TYPE_VALUES \
    DEVICE_METHOD_INVOKED, \
    DEVICE_METHOD_RECEIVED

MU_DEFINE_ENUM(DEVICE_METHOD_EVENT_TYPE, DEVICE_METHOD_EVENT_TYPE_VALUES)

#define DEVICE_TWIN_EVENT_TYPE_VALUES \
    DEVICE_TWIN_UPDATE_QUEUED, \
    DEVICE_TWIN_UPDATE_SENT, \
    DEVICE_TWIN_UPDATE_RECEIVED

MU_DEFINE_ENUM(DEVICE_TWIN_EVENT_TYPE, DEVICE_TWIN_EVENT_TYPE_VALUES)

typedef struct TELEMETRY_INFO_TAG
{
    size_t message_id;

    time_t time_queued;
    size_t send_result;

    time_t time_sent;
    IOTHUB_CLIENT_CONFIRMATION_RESULT send_callback_result;

    time_t time_received;
} TELEMETRY_INFO;

typedef struct IOTHUB_CLIENT_STATISTICS_TELEMETRY_SUMMARY_TAG
{
    size_t messages_sent;
    size_t messages_received;
    double min_travel_time_secs;
    double max_travel_time_secs;
} IOTHUB_CLIENT_STATISTICS_TELEMETRY_SUMMARY;

typedef struct C2D_MESSAGE_INFO_TAG
{
    size_t message_id;

    time_t time_queued;
    size_t send_result;

    time_t time_sent;
    IOTHUB_MESSAGING_RESULT send_callback_result;

    time_t time_received;
} C2D_MESSAGE_INFO;

typedef struct IOTHUB_CLIENT_STATISTICS_C2D_SUMMARY_TAG
{
    size_t messages_sent;
    size_t messages_received;
    double min_travel_time_secs;
    double max_travel_time_secs;
} IOTHUB_CLIENT_STATISTICS_C2D_SUMMARY;

typedef struct DEVICE_METHOD_INFO_TAG
{
    size_t method_id;

    time_t time_invoked;
    IOTHUB_DEVICE_METHOD_RESULT method_result;

    time_t time_received;
} DEVICE_METHOD_INFO;

typedef struct IOTHUB_CLIENT_STATISTICS_DEVICE_METHOD_SUMMARY_TAG
{
    size_t methods_invoked;
    size_t methods_received;
    double min_travel_time_secs;
    double max_travel_time_secs;
} IOTHUB_CLIENT_STATISTICS_DEVICE_METHOD_SUMMARY;

typedef struct DEVICE_TWIN_DESIRED_INFO_TAG
{
    size_t update_id;

    time_t time_updated;
    int update_result;

    time_t time_received;
} DEVICE_TWIN_DESIRED_INFO;

typedef struct DEVICE_TWIN_REPORTED_INFO_TAG
{
    size_t update_id;

    time_t time_queued;
    IOTHUB_CLIENT_RESULT update_result;

    time_t time_sent;
    int send_status_code;

    time_t time_received;
} DEVICE_TWIN_REPORTED_INFO;

typedef struct IOTHUB_CLIENT_STATISTICS_DEVICE_TWIN_SUMMARY_TAG
{
    size_t updates_sent;
    size_t updates_received;
    double min_travel_time_secs;
    double max_travel_time_secs;
} IOTHUB_CLIENT_STATISTICS_DEVICE_TWIN_SUMMARY;

typedef struct IOTHUB_CLIENT_STATISTICS_TAG* IOTHUB_CLIENT_STATISTICS_HANDLE;

extern IOTHUB_CLIENT_STATISTICS_HANDLE iothub_client_statistics_create(void);

extern char* iothub_client_statistics_to_json(IOTHUB_CLIENT_STATISTICS_HANDLE handle);

extern int iothub_client_statistics_add_connection_status(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_CONNECTION_STATUS status, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);

extern int iothub_client_statistics_add_telemetry_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, TELEMETRY_EVENT_TYPE type, TELEMETRY_INFO* info);

extern int iothub_client_statistics_get_telemetry_summary(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_STATISTICS_TELEMETRY_SUMMARY* summary);

extern int iothub_client_statistics_add_c2d_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, C2D_EVENT_TYPE type, C2D_MESSAGE_INFO* info);

extern int iothub_client_statistics_get_c2d_summary(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_STATISTICS_C2D_SUMMARY* summary);

extern int iothub_client_statistics_add_device_method_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, DEVICE_METHOD_EVENT_TYPE type, DEVICE_METHOD_INFO* info);

extern int iothub_client_statistics_get_device_method_summary(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_STATISTICS_DEVICE_METHOD_SUMMARY* summary);

extern int iothub_client_statistics_add_device_twin_desired_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, DEVICE_TWIN_EVENT_TYPE type, DEVICE_TWIN_DESIRED_INFO* info);

extern int iothub_client_statistics_get_device_twin_desired_summary(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_STATISTICS_DEVICE_TWIN_SUMMARY* summary);

extern int iothub_client_statistics_add_device_twin_reported_info(IOTHUB_CLIENT_STATISTICS_HANDLE handle, DEVICE_TWIN_EVENT_TYPE type, DEVICE_TWIN_REPORTED_INFO* info);

extern int iothub_client_statistics_get_device_twin_reported_summary(IOTHUB_CLIENT_STATISTICS_HANDLE handle, IOTHUB_CLIENT_STATISTICS_DEVICE_TWIN_SUMMARY* summary);

extern void iothub_client_statistics_destroy(IOTHUB_CLIENT_STATISTICS_HANDLE handle);

#endif // IOTHUB_CLIENT_STATISTICS_H
