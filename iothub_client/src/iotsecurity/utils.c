#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/xlogging.h"
#include "iotsecurity/utils.h"


#define MAX_LENGTH_MACHINE_ID 512

static const char* DATETIME_FORMAT = "%FT%TZ";


STRING_HANDLE OSUtils_GetMachineId() {
    /*
    * Get machine-id
    *
    * The /etc/machine-id file contains the unique machine ID of the local
    * system that is set during installation. The machine ID is a single
    * newline-terminated, hexadecimal, 32-character, lowercase ID. When
    * decoded from hexadecimal, this corresponds to a 16-byte/128-bit
    * value.

    * The machine ID is usually generated from a random source during
    * system installation and stays constant for all subsequent boots.
    * Optionally, for stateless systems, it is generated during runtime at
    * early boot if it is found to be empty.
    *
    */
    STRING_HANDLE machineId = NULL;
    FILE *fp = NULL;
    char line[MAX_LENGTH_MACHINE_ID] = { 0 };

    fp = popen("cat /etc/machine-id", "r");
    if (fp == NULL) {
        goto cleanup;
    }

    if (!ferror(fp)) {
        if (fgets(line, sizeof(line), fp) != NULL) {
            machineId = STRING_construct_n((const char*)line, strlen(line) - 1);
            if (machineId == NULL) {
                goto cleanup;
            }
        }
    }

cleanup:
    if (fp != NULL) {
        pclose(fp);
    }

    if (machineId == NULL) {
        LogInfo("WARNING: failed to find machine-id.");
    }

    return machineId;
}


bool TimeUtils_GetTimeAsString(time_t* currentTime, char* output, uint32_t* outputSize) {
    struct tm currentLocalTime;
    if (localtime_r(currentTime, &currentLocalTime) == NULL) {
        return false;
    }
    *outputSize = strftime(output, *outputSize, DATETIME_FORMAT, &currentLocalTime);
    return true;
}


bool TimeUtils_GetLocalTimeAsUTCTimeAsString(time_t* currentLocalTime, char* output, uint32_t* outputSize) {
    struct tm currentUTCTime;
    if (gmtime_r(currentLocalTime, &currentUTCTime) == NULL) {
        return false;
    }
    *outputSize = strftime(output, *outputSize, DATETIME_FORMAT, &currentUTCTime);
    return true;
}