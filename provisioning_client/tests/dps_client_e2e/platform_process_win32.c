// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "platform_process.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"

#include <Windows.h>

typedef struct PLATFORM_PROCESS_INFO_TAG
{
    char* iothub_uri;
    PROCESS_INFORMATION proc_info;
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
        char directory_info[128];
        GetCurrentDirectory(128, directory_info);

        STARTUPINFO si;
        memset(result, 0, sizeof(PLATFORM_PROCESS_INFO));
        memset(&si, 0, sizeof(STARTUPINFO));

        if (!CreateProcessA(NULL, (char*)command_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &result->proc_info))
        {
            LogError("Failure launch program 0x%x:%s", GetLastError(), command_line);
            free(result);
            result = NULL;
        }
        else
        {

        }
    }
    return result;
}

void platform_process_destroy(PLATFORM_PROCESS_HANDLE handle)
{
    if (handle != NULL)
    {
        PLATFORM_PROCESS_INFO* proc_info = (PLATFORM_PROCESS_INFO*)handle;
        if (TerminateProcess(proc_info->proc_info.hProcess, 0))
        {
            WaitForSingleObject(proc_info->proc_info.hProcess, 5000);
        }
        CloseHandle(proc_info->proc_info.hProcess);
        CloseHandle(proc_info->proc_info.hThread);
        free(proc_info);
    }
}
