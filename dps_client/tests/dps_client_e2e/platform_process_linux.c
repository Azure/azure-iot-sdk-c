// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <unistd.h>

#include "platform_process.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"

typedef struct PLATFORM_PROCESS_INFO_TAG
{
    pid_t proc_id;
} PLATFORM_PROCESS_INFO;

PLATFORM_PROCESS_HANDLE platform_process_create(const char* command_line)
{
    PLATFORM_PROCESS_INFO* result;
    if ((result = (PLATFORM_PROCESS_INFO*)malloc(sizeof(PLATFORM_PROCESS_INFO))) == NULL)
    {
        LogError("Failure allocating platform info");
    }
    else
    {
        result->proc_id = fork();
        if (result->proc_id == 0)
        {
            // Child process
            execl(command_line, command_line, (char*)0);
        }
        else if (result->proc_id < 0)
        {
            // Failure
            free(result);
            result = NULL;
            LogError("Failure forking child process");
        }
        else
        {
            // parent process
        }
    }
    return result;
}

void platform_process_destroy(PLATFORM_PROCESS_HANDLE handle)
{
    if (handle != NULL)
    {
        PLATFORM_PROCESS_INFO* proc_info = (PLATFORM_PROCESS_INFO*)handle;
        kill(proc_info->proc_id, SIGTERM);
        ThreadAPI_sleep(2);
        kill(proc_info->proc_id, SIGKILL);
        free(proc_info);
    }
}
