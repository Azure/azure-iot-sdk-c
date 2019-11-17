#include <stdlib.h>
#include "iothub_client.h"
#include "parson.h"
#include "string.h"
#include "iotsecurity/utils.h"
#include "iotsecurity/message_schema_consts.h"
#include "iotsecurity/collector.h"
#include "iotsecurity/collector_network.h"

#define COMMAND_SS_MIN_SCAN_COLS 5

#define RECORD_VALUE_MAX_LENGTH 512
#define RECORD_TIME_PROPERTY_MAX_LENGTH 25

/*
* FIXME RECORD_PROPERTY_MAX_LENGTH_NETID and RECORD_PROPERTY_MAX_LENGTH_STATE
* has been set following https://github.com/sivasankariit/iproute2/blob/master/misc/ss.c
* while the rest are set arbitrary
*/
#define RECORD_PROPERTY_MAX_LENGTH_NETID 13
#define RECORD_PROPERTY_MAX_LENGTH_STATE 13
#define RECORD_PROPERTY_MAX_LENGTH_LOCAL_ADDRESS 256
#define RECORD_PROPERTY_MAX_LENGTH_PEER_ADDRESS 256
#define RECORD_PROPERTY_MAX_LENGTH_METADATA 256


MU_DEFINE_ENUM_STRINGS(NETID_RESULT, NETID_RESULT_VALUES);

/*
* TODO support more socket types i.e. raw, unix and etc
*/
static const char* NETWORK_COMMAND = "ss --no-header --tcp --udp --numeric --options --processes";
static const char* NETWORK_COMMAND_SS_FORMAT = "%s\t%s\t%d\t%d\t%s\t%s\t%s\n";

static STRING_HANDLE MACHINE_ID = NULL;


CollectorResult CollectorNetwork(JSON_Object *root);

CollectorResult CollectorNetwork_AddMetadata(JSON_Object *root);

CollectorResult CollectorNetwork_AddRecord(JSON_Array *payload, char* line);


CollectorResult CollectorNetwork(JSON_Object *root) {
    CollectorResult collector_result = COLLECTOR_OK;
    JSON_Status json_status = JSONSuccess;

    FILE *fp;
    char line[RECORD_VALUE_MAX_LENGTH] = { 0 };

    collector_result = CollectorNetwork_AddMetadata(root);
    if (collector_result != COLLECTOR_OK) {
        goto cleanup;
    }

    // Open the command output for reading
    fp = popen(NETWORK_COMMAND, "r");
    if (fp == NULL) {
        collector_result = COLLECTOR_EXCEPTION;
        goto cleanup;
    }

    if (ferror(fp)) {
        collector_result = COLLECTOR_EXCEPTION;
        goto cleanup;
    }

    JSON_Array *payload_array = json_object_get_array(root, PAYLOAD_KEY);

    while (fgets(line, sizeof(line), fp) != NULL) {
        CollectorResult add_record_result = COLLECTOR_OK;
        add_record_result = CollectorNetwork_AddRecord(payload_array, line);
        if (add_record_result != COLLECTOR_OK) {
            // TODO add warning
            continue;
        }
    }

    // TODO fill IsEmpty

cleanup:
    if (fp != NULL) {
        pclose(fp);
    }

    if (collector_result != COLLECTOR_OK) {
        // TODO free
    }

    return collector_result;
}

CollectorResult CollectorNetwork_AddMetadata(JSON_Object *root) {
    CollectorResult collector_result = COLLECTOR_OK;
    JSON_Status json_status = JSONSuccess;

    json_status = json_object_set_string(root, MESSAGE_SCHEMA_VERSION_KEY, LISTENING_PORTS_PAYLOAD_SCHEMA_VERSION);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

    json_status = json_object_set_string(root, EVENT_NAME_KEY, LISTENING_PORTS_NAME);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

    json_status = json_object_set_string(root, EVENT_CATEGORY_KEY, EVENT_PERIODIC_CATEGORY);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

    json_status = json_object_set_string(root, EVENT_TYPE_KEY, EVENT_TYPE_SECURITY_VALUE);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

    MACHINE_ID = OSUtils_GetMachineId();
    if (MACHINE_ID != NULL) {
        json_status = json_object_set_string(root, EVENT_ID_KEY, STRING_c_str(MACHINE_ID));
        if (json_status != JSONSuccess) {
            goto cleanup;
        }
    }

    // TODO timings
    // char timeStr[RECORD_TIME_PROPERTY_MAX_LENGTH];
    // uint32_t timeStrLength = RECORD_TIME_PROPERTY_MAX_LENGTH;
    // memset(timeStr, 0, timeStrLength);
    // if (!TimeUtils_GetTimeAsString(eventLocalTime, timeStr, &timeStrLength)) {
    //     return EVENT_COLLECTOR_EXCEPTION;
    // }
    // json_status = json_object_set_string(root, EVENT_LOCAL_TIMESTAMP_KEY, xxx);
    // if (json_status != JSONSuccess) {
    //     goto cleanup;
    // }

    // json_status = json_object_set_string(root, EVENT_UTC_TIMESTAMP_KEY, xxx);
    // if (json_status != JSONSuccess) {
    //     goto cleanup;
    // }

    JSON_Value *payload_value = json_value_init_array();
    JSON_Array *payload_object = json_value_get_array(payload_value);

    json_status = json_object_set_value(root, PAYLOAD_KEY, payload_value);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

cleanup:
    if (json_status != JSONSuccess) {
        collector_result = COLLECTOR_EXCEPTION;
    }

    if (collector_result != COLLECTOR_OK) {
        // TODO free
    }

    return collector_result;
}

CollectorResult CollectorNetwork_AddRecord(JSON_Array *payload, char* line) {
    /*
     * Parse line and append to payload array
     */
    CollectorResult collector_result = COLLECTOR_OK;
    JSON_Status json_status = JSONSuccess;

    // TODO make it struct
    char netid[13] = { 0 };
    char state[RECORD_PROPERTY_MAX_LENGTH_STATE] = { 0 };
    unsigned int recvQ;
    unsigned int sendQ;
    char localAddress[RECORD_PROPERTY_MAX_LENGTH_LOCAL_ADDRESS] = { 0 };
    char peerAddress[RECORD_PROPERTY_MAX_LENGTH_PEER_ADDRESS] = { 0 };
    char metadata[RECORD_PROPERTY_MAX_LENGTH_METADATA] = { 0 };

    int scanCols = sscanf(line, NETWORK_COMMAND_SS_FORMAT, netid, state, &recvQ, &sendQ, localAddress, peerAddress, metadata);
    if (scanCols < COMMAND_SS_MIN_SCAN_COLS) {
        collector_result = COLLECTOR_PARSE_EXCEPTION;
        goto cleanup;
    }

    JSON_Value *record_value = json_value_init_object();
    JSON_Object *record_object = json_value_get_object(record_value);

    NETID_RESULT netid_result;
    MU_STRING_TO_ENUM("NETID_TCP", NETID_RESULT, &netid_result);
    switch (netid_result)
    {
        case NETID_TCP:
        case NETID_UDP:
            json_status = json_object_set_string(record_object, LISTENING_PORTS_PROTOCOL_KEY, netid);
            if (json_status != JSONSuccess) {
                goto cleanup;
            }
            break;
        case NETID_RAW:
        case NETID_U_DGR:
        case NETID_U_STR:
        case NETID_P_RAW:
        case NETID_P_DGR:
        case NETID_NL:
        default:
            collector_result = COLLECTOR_NOT_SUPPORTED_EXCEPTION;
            goto cleanup;
    }

    /*
    * TODO LISTENING_PORTS_LOCAL_ADDRESS_KEY, LISTENING_PORTS_LOCAL_PORT_KEY
    */
    json_status = json_object_set_string(record_object, "Local Address:Port", localAddress);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

    /*
    * TODO LISTENING_PORTS_REMOTE_ADDRESS_KEY, LISTENING_PORTS_REMOTE_PORT_KEY
    */
    json_status = json_object_set_string(record_object, "Peer Address:Port", peerAddress);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

    json_status = json_object_set_string(record_object, "State", state);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

    json_status = json_object_set_number(record_object, "Recv-Q", recvQ);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

    json_status = json_object_set_number(record_object, "Send-Q", sendQ);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

    json_status = json_object_set_string(record_object, "Metadata", metadata);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

    json_status = json_array_append_value(payload, record_value);
    if (json_status != JSONSuccess) {
        goto cleanup;
    }

cleanup:
    if (json_status != JSONSuccess) {
        collector_result = COLLECTOR_EXCEPTION;
    }

    if (collector_result != COLLECTOR_OK) {
        // TODO free
    }

    return collector_result;
}


