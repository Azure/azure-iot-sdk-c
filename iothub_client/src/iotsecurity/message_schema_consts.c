// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iotsecurity/message_schema_consts.h"

const char* PAYLOAD_KEY = "Payload";
const char* EVENTS_KEY = "Events";
const char* AGENT_VERSION_KEY = "AgentVersion";
const char* AGENT_ID_KEY = "AgentId";
const char* MESSAGE_SCHEMA_VERSION_KEY = "MessageSchemaVersion";
const char* HUB_RESOURCE_ID_PROPERTY_KEY = "HubResourceId";
const char* EXTRA_DETAILS_KEY = "ExtraDetails";

const char* EVENT_CATEGORY_KEY = "Category";
const char* EVENT_PERIODIC_CATEGORY = "Periodic";
const char* EVENT_TRIGGERED_CATEGORY = "Triggered";
const char* EVENT_AGGREGATED_CATEGORY = "Aggregated";
const char* EVENT_IS_EMPTY_KEY = "IsEmpty";
const char* EVENT_NAME_KEY = "Name";
const char* EVENT_TYPE_KEY = "EventType";
const char* EVENT_PAYLOAD_SCHEMA_VERSION_KEY = "PayloadSchemaVersion";
const char* EVENT_ID_KEY = "Id";
const char* EVENT_LOCAL_TIMESTAMP_KEY = "TimestampLocal";
const char* EVENT_UTC_TIMESTAMP_KEY = "TimestampUTC";
const char* EVENT_TYPE_SECURITY_VALUE = "Security";
const char* EVENT_TYPE_OPERATIONAL_VALUE = "Operational";
const char* EVENT_TYPE_DIAGNOSTIC_VALUE = "Diagnostic";


const char* PROCESS_CREATION_NAME = "ProcessCreate";
const char* PROCESS_CREATION_PAYLOAD_SCHEMA_VERSION = "1.0";
const char* PROCESS_CREATION_EXECUTABLE_KEY = "Executable";
const char* PROCESS_CREATION_PROCESS_ID_KEY = "ProcessId";
const char* PROCESS_CREATION_PARENT_PROCESS_ID_KEY = "ParentProcessId";
const char* PROCESS_CREATION_USER_ID_KEY = "UserId";
const char* PROCESS_CREATION_COMMAND_LINE_KEY = "CommandLine";


const char* LOCAL_USERS_NAME = "LocalUsers";
const char* LOCAL_USERS_PAYLOAD_SCHEMA_VERSION = "1.0";
const char* LOCAL_USERS_USER_NAME_KEY = "UserName";
const char* LOCAL_USERS_USER_ID_KEY = "UserId";
const char* LOCAL_USERS_GROUP_NAMES_KEY = "GroupNames";
const char* LOCAL_USERS_GROUP_IDS_KEY = "GroupIds";
const char* LOCAL_USERS_PAYLOAD_DELIMITER = ";";


const char* USER_LOGIN_NAME = "Login";
const char* USER_LOGIN_PAYLOAD_SCHEMA_VERSION = "1.0";
const char* USER_LOGIN_PROCESS_ID_KEY = "ProcessId";
const char* USER_LOGIN_USER_ID_KEY = "UserId";
const char* USER_LOGIN_USERNAME_KEY = "UserName";
const char* USER_LOGIN_EXECUTABLE_KEY = "Executable";
const char* USER_LOGIN_RESULT_KEY = "Result";
const char* USER_LOGIN_RESULT_SUCCESS_VALUE = "Success";
const char* USER_LOGIN_RESULT_FAILED_VALUE = "Fail";
const char* USER_LOGIN_REMOTE_ADDRESS_KEY = "RemoteAddress";
const char* USER_LOGIN_OPERATION_KEY = "Operation";


const char* CONNECTION_CREATION_NAME = "ConnectionCreate";
const char* CONNECTION_CREATION_PAYLOAD_SCHEMA_VERSION = "1.0";
const char* CONNECTION_CREATION_EXECUTABLE_KEY = "Executable";
const char* CONNECTION_CREATION_COMMAND_LINE_KEY = "CommandLine";
const char* CONNECTION_CREATION_PROCESS_ID_KEY = "ProcessId";
const char* CONNECTION_CREATION_USER_ID_KEY = "UserId";
const char* CONNECTION_CREATION_PROTOCOL_KEY = "Protocol";
const char* CONNECTION_CREATION_REMOTE_ADDRESS_KEY = "RemoteAddress";
const char* CONNECTION_CREATION_REMOTE_PORT_KEY = "RemotePort";
const char* CONNECTION_CREATION_DIRECTION_KEY = "Direction";
const char* CONNECTION_CREATION_DIRECTION_INBOUND_NAME = "In";
const char* CONNECTION_CREATION_DIRECTION_OUTBOUND_NAME = "Out";



const char* DIAGNOSTIC_MESSAGE_KEY = "Message";
const char* DIAGNOSTIC_SEVERITY_KEY = "Severity";
const char* DIAGNOSTIC_PROCESSID_KEY = "ProcessId";
const char* DIAGNOSTIC_THREAD_KEY = "ThreadId";
const char* DIAGNOSTIC_CORRELATION_KEY = "CorrelationId";
const char* DIAGNOSTIC_SEVERITY_DEBUG_VALUE = "Debug";
const char* DIAGNOSTIC_SEVERITY_INFORMATION_VALUE = "Information";
const char* DIAGNOSTIC_SEVERITY_WARNING_VALUE = "Warning";
const char* DIAGNOSTIC_SEVERITY_ERROR_VALUE = "Error";
const char* DIAGNOSTIC_SEVERITY_FATAL_VALUE = "Fatal";
const char* DIAGNOSTIC_NAME = "Diagnostic";
const char* DIAGNOSTIC_PAYLOAD_SCHEMA_VERSION = "1.0";

const char* AGENT_TELEMETRY_COLLECTED_EVENTS_KEY = "CollectedEvents";
const char* AGENT_TELEMETRY_DROPPED_EVENTS_KEY = "DroppedEvents";
const char* AGENT_TELEMETRY_DROPPED_EVENTS_NAME = "DroppedEventsStatistics";
const char* AGENT_TELEMETRY_DROPPED_EVENTS_SCHEMA_VERSION = "1.0";
const char* AGENT_TELEMETRY_MESSAGE_STATISTICS_NAME = "MessageStatistics";
const char* AGENT_TELEMETRY_MESSAGE_STATISTICS_SCHEMA_VERSION = "1.0";
const char* AGENT_TELEMETRY_MESSAGES_FAILED_KEY = "TotalFailed";
const char* AGENT_TELEMETRY_MESSAGES_SENT_KEY = "MessagesSent";
const char* AGENT_TELEMETRY_MESSAGES_UNDER_4KB_KEY = "MessagesUnder4KB";
const char* AGENT_TELEMETRY_QUEUE_EVENTS_KEY = "Queue";

const char* AGENT_CONFIGURATION_ERROR_CONFIGURATION_NAME_KEY = "ConfigurationName";
const char* AGENT_CONFIGURATION_ERROR_ERROR_KEY = "ErrorType";
const char* AGENT_CONFIGURATION_ERROR_EVENT_NAME = "ConfigurationError";
const char* AGENT_CONFIGURATION_ERROR_EVENT_SCHEMA_VERSION = "1.0";
const char* AGENT_CONFIGURATION_ERROR_MESSAGE_KEY = "Message";
const char* AGENT_CONFIGURATION_ERROR_USED_CONFIGURATION_KEY = "UsedConfiguration";

const char* SYSTEM_INFORMATION_NAME = "SystemInformation";
const char* SYSTEM_INFORMATION_PAYLOAD_SCHEMA_VERSION = "1.0";
const char* SYSTEM_INFORMATION_OS_NAME_KEY = "OSName";
const char* SYSTEM_INFORMATION_OS_VERSION_KEY = "OSVersion";
const char* SYSTEM_INFORMATION_OS_ARCHITECTURE_KEY = "OsArchitecture";
const char* SYSTEM_INFORMATION_HOST_NAME_KEY = "HostName";
const char* SYSTEM_INFORMATION_TOTAL_PHYSICAL_MEMORY_KEY = "TotalPhysicalMemoryInKB";
const char* SYSTEM_INFORMATION_FREE_PHYSICAL_MEMORY_KEY = "FreePhysicalMemoryInKB";


const char* LISTENING_PORTS_NAME = "ListeningPorts";
const char* LISTENING_PORTS_PAYLOAD_SCHEMA_VERSION = "1.0";
const char* LISTENING_PORTS_PROTOCOL_KEY = "Protocol";
const char* LISTENING_PORTS_LOCAL_ADDRESS_KEY = "LocalAddress";
const char* LISTENING_PORTS_LOCAL_PORT_KEY = "LocalPort";
const char* LISTENING_PORTS_REMOTE_ADDRESS_KEY = "RemoteAddress";
const char* LISTENING_PORTS_REMOTE_PORT_KEY = "RemotePort";
const char* LISTENING_PORTS_PID_KEY = "Pid";


const char* FIREWALL_RULES_NAME = "FirewallConfiguration";
const char* FIREWALL_RULES_PAYLOAD_SCHEMA_VERSION = "1.0";
const char* FIREWALL_RULES_ENABLED_KEY = "Enabled";
const char* FIREWALL_RULES_ACTION_KEY = "Action";
const char* FIREWALL_RULES_PROTOCOL_KEY = "Protocol";
const char* FIREWALL_RULES_CHAIN_NAME_KEY = "ChainName";
const char* FIREWALL_RULES_DIRECTION_KEY = "Direction";
const char* FIREWALL_RULES_PRIORITY_KEY = "Priority";
const char* FIREWALL_RULES_SRC_ADDRESS_KEY = "SourceAddress";
const char* FIREWALL_RULES_SRC_PORT_KEY = "SourcePort";
const char* FIREWALL_RULES_DEST_ADDRESS_KEY = "DestinationAddress";
const char* FIREWALL_RULES_DEST_PORT_KEY = "DestinationPort";


const char* BASELINE_NAME = "OSBaseline";
const char* BASELINE_PAYLOAD_SCHEMA_VERSION = "1.0";
const char* BASELINE_DESCRIPTION_KEY = "Description";
const char* BASELINE_CCEID_KEY = "CceId";
const char* BASELINE_RESULT_KEY = "Result";
const char* BASELINE_ERROR_KEY = "Error";
const char* BASELINE_SEVERITY_KEY = "Severity";
