// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MESSAGE_SCHEMA_H
#define MESSAGE_SCHEMA_H

#include <stdbool.h>

/* ===== Security Message Message Schema =====*/

extern const char* PAYLOAD_KEY;
extern const char* EVENTS_KEY;
extern const char* AGENT_VERSION_KEY;
extern const char* AGENT_ID_KEY;
extern const char* MESSAGE_SCHEMA_VERSION_KEY;
extern const char* HUB_RESOURCE_ID_PROPERTY_KEY;
extern const char* EXTRA_DETAILS_KEY;

/* ===== Generic Event Message Schema =====*/

extern const char* EVENT_CATEGORY_KEY;
extern const char* EVENT_PERIODIC_CATEGORY;
extern const char* EVENT_TRIGGERED_CATEGORY;
extern const char* EVENT_AGGREGATED_CATEGORY;
extern const char* EVENT_IS_EMPTY_KEY;
extern const char* EVENT_NAME_KEY;
extern const char* EVENT_TYPE_KEY;
extern const char* EVENT_PAYLOAD_SCHEMA_VERSION_KEY;
extern const char* EVENT_ID_KEY;
extern const char* EVENT_LOCAL_TIMESTAMP_KEY;
extern const char* EVENT_UTC_TIMESTAMP_KEY;
extern const char* EVENT_TYPE_SECURITY_VALUE;
extern const char* EVENT_TYPE_OPERATIONAL_VALUE;
extern const char* EVENT_TYPE_DIAGNOSTIC_VALUE;

/* ===== Process Creation Message Schema =====*/

extern const char* PROCESS_CREATION_NAME;
extern const char* PROCESS_CREATION_PAYLOAD_SCHEMA_VERSION;
extern const char* PROCESS_CREATION_EXECUTABLE_KEY;
extern const char* PROCESS_CREATION_PROCESS_ID_KEY;
extern const char* PROCESS_CREATION_PARENT_PROCESS_ID_KEY;
extern const char* PROCESS_CREATION_USER_ID_KEY;
extern const char* PROCESS_CREATION_COMMAND_LINE_KEY;

/* ===== Connection Creation Message Schema =====*/

extern const char* CONNECTION_CREATION_NAME;
extern const char* CONNECTION_CREATION_PAYLOAD_SCHEMA_VERSION;
extern const char* CONNECTION_CREATION_PROCESS_ID_KEY;
extern const char* CONNECTION_CREATION_COMMAND_LINE_KEY;
extern const char* CONNECTION_CREATION_EXECUTABLE_KEY;
extern const char* CONNECTION_CREATION_USER_ID_KEY;
extern const char* CONNECTION_CREATION_PROTOCOL_KEY;
extern const char* CONNECTION_CREATION_REMOTE_ADDRESS_KEY;
extern const char* CONNECTION_CREATION_REMOTE_PORT_KEY;
extern const char* CONNECTION_CREATION_DIRECTION_KEY;
extern const char* CONNECTION_CREATION_DIRECTION_INBOUND_NAME;
extern const char* CONNECTION_CREATION_DIRECTION_OUTBOUND_NAME;

/* ===== Local users Message Schema =====*/

extern const char* LOCAL_USERS_NAME;
extern const char* LOCAL_USERS_PAYLOAD_SCHEMA_VERSION;
extern const char* LOCAL_USERS_USER_NAME_KEY;
extern const char* LOCAL_USERS_USER_ID_KEY;
extern const char* LOCAL_USERS_GROUP_NAMES_KEY;
extern const char* LOCAL_USERS_GROUP_IDS_KEY;
extern const char* LOCAL_USERS_PAYLOAD_DELIMITER;

/* ===== User Login Message Schema =====*/

extern const char* USER_LOGIN_NAME;
extern const char* USER_LOGIN_PAYLOAD_SCHEMA_VERSION;
extern const char* USER_LOGIN_PROCESS_ID_KEY;
extern const char* USER_LOGIN_USER_ID_KEY;
extern const char* USER_LOGIN_USERNAME_KEY;
extern const char* USER_LOGIN_EXECUTABLE_KEY;
extern const char* USER_LOGIN_RESULT_KEY;
extern const char* USER_LOGIN_RESULT_SUCCESS_VALUE;
extern const char* USER_LOGIN_RESULT_FAILED_VALUE;
extern const char* USER_LOGIN_OPERATION_KEY;
extern const char* USER_LOGIN_REMOTE_ADDRESS_KEY;

/* ===== Diagnostic Message Schema =====*/

extern const char* DIAGNOSTIC_MESSAGE_KEY;
extern const char* DIAGNOSTIC_SEVERITY_KEY;
extern const char* DIAGNOSTIC_PROCESSID_KEY;
extern const char* DIAGNOSTIC_THREAD_KEY;
extern const char* DIAGNOSTIC_CORRELATION_KEY;
extern const char* DIAGNOSTIC_SEVERITY_DEBUG_VALUE;
extern const char* DIAGNOSTIC_SEVERITY_INFORMATION_VALUE;
extern const char* DIAGNOSTIC_SEVERITY_WARNING_VALUE;
extern const char* DIAGNOSTIC_SEVERITY_ERROR_VALUE;
extern const char* DIAGNOSTIC_SEVERITY_FATAL_VALUE;
extern const char* DIAGNOSTIC_NAME;
extern const char* DIAGNOSTIC_PAYLOAD_SCHEMA_VERSION;

/* ===== Dropped Events Message Schema =====*/

extern const char* AGENT_TELEMETRY_COLLECTED_EVENTS_KEY;
extern const char* AGENT_TELEMETRY_DROPPED_EVENTS_KEY;
extern const char* AGENT_TELEMETRY_QUEUE_EVENTS_KEY;
extern const char* AGENT_TELEMETRY_DROPPED_EVENTS_NAME;
extern const char* AGENT_TELEMETRY_DROPPED_EVENTS_SCHEMA_VERSION;
extern const char* AGENT_TELEMETRY_MESSAGE_STATISTICS_NAME;
extern const char* AGENT_TELEMETRY_MESSAGE_STATISTICS_SCHEMA_VERSION;
extern const char* AGENT_TELEMETRY_MESSAGES_SENT_KEY;
extern const char* AGENT_TELEMETRY_MESSAGES_FAILED_KEY;
extern const char* AGENT_TELEMETRY_MESSAGES_UNDER_4KB_KEY;

/* ===== Configuration Error Message Schema ====*/

extern const char* AGENT_CONFIGURATION_ERROR_EVENT_NAME;
extern const char* AGENT_CONFIGURATION_ERROR_EVENT_SCHEMA_VERSION ;
extern const char* AGENT_CONFIGURATION_ERROR_CONFIGURATION_NAME_KEY;
extern const char* AGENT_CONFIGURATION_ERROR_USED_CONFIGURATION_KEY;
extern const char* AGENT_CONFIGURATION_ERROR_MESSAGE_KEY;
extern const char* AGENT_CONFIGURATION_ERROR_ERROR_KEY;

/* ===== System Information Message Schema =====*/

extern const char* SYSTEM_INFORMATION_NAME;
extern const char* SYSTEM_INFORMATION_PAYLOAD_SCHEMA_VERSION;
extern const char* SYSTEM_INFORMATION_OS_NAME_KEY;
extern const char* SYSTEM_INFORMATION_OS_VERSION_KEY;
extern const char* SYSTEM_INFORMATION_OS_ARCHITECTURE_KEY;
extern const char* SYSTEM_INFORMATION_HOST_NAME_KEY;
extern const char* SYSTEM_INFORMATION_TOTAL_PHYSICAL_MEMORY_KEY;
extern const char* SYSTEM_INFORMATION_FREE_PHYSICAL_MEMORY_KEY;

/* ===== Listening Ports Message Schema =====*/

extern const char* LISTENING_PORTS_NAME;
extern const char* LISTENING_PORTS_PAYLOAD_SCHEMA_VERSION;
extern const char* LISTENING_PORTS_PROTOCOL_KEY;
extern const char* LISTENING_PORTS_LOCAL_ADDRESS_KEY;
extern const char* LISTENING_PORTS_LOCAL_PORT_KEY;
extern const char* LISTENING_PORTS_REMOTE_ADDRESS_KEY;
extern const char* LISTENING_PORTS_REMOTE_PORT_KEY;
extern const char* LISTENING_PORTS_PID_KEY;

/* ===== Firewall Rules Message Schema =====*/

extern const char* FIREWALL_RULES_NAME;
extern const char* FIREWALL_RULES_PAYLOAD_SCHEMA_VERSION;
extern const char* FIREWALL_RULES_ENABLED_KEY;
extern const char* FIREWALL_RULES_ACTION_KEY;
extern const char* FIREWALL_RULES_CHAIN_NAME_KEY;
extern const char* FIREWALL_RULES_DIRECTION_KEY;
extern const char* FIREWALL_RULES_PRIORITY_KEY;
extern const char* FIREWALL_RULES_PROTOCOL_KEY;
extern const char* FIREWALL_RULES_SRC_ADDRESS_KEY;
extern const char* FIREWALL_RULES_SRC_PORT_KEY;
extern const char* FIREWALL_RULES_DEST_ADDRESS_KEY;
extern const char* FIREWALL_RULES_DEST_PORT_KEY;

/* ===== Baseline Message Schema =====*/

extern const char* BASELINE_NAME;
extern const char* BASELINE_PAYLOAD_SCHEMA_VERSION;
extern const char* BASELINE_DESCRIPTION_KEY;
extern const char* BASELINE_CCEID_KEY;
extern const char* BASELINE_RESULT_KEY;
extern const char* BASELINE_ERROR_KEY;
extern const char* BASELINE_SEVERITY_KEY;

#endif //MESSAGE_SCHEMA_H