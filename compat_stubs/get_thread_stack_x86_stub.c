// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Stub for Win32 x86: get_thread_stack.c requires RtlVirtualUnwind which
// is only available on x64/ARM64.  Provide no-op implementations so that
// c_logging_v2 links on x86.

#if defined(_MSC_VER) && defined(_M_IX86)

#include <stdlib.h>
#include <stdio.h>
#include "windows.h"
#include "c_logging/get_thread_stack.h"

int get_thread_stack_init(void)
{
    return 0;
}

void get_thread_stack(DWORD threadId, char* destination, size_t destinationSize)
{
    (void)threadId;
    if (destination != NULL && destinationSize > 0)
    {
        (void)snprintf(destination, destinationSize, "stack capture not supported on x86");
    }
}

void get_thread_stack_refresh_module_list(void)
{
}

void get_thread_stack_deinit(void)
{
}

#endif
