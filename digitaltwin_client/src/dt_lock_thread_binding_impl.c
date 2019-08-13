// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "internal/dt_lock_thread_binding_impl.h"

LOCK_HANDLE DT_LockBinding_LockInit_Impl(void)
{
    return Lock_Init();
}

LOCK_RESULT DT_LockBinding_Lock_Impl(LOCK_HANDLE bindingLock)
{
    return Lock(bindingLock);
}

LOCK_RESULT DT_LockBinding_Unlock_Impl(LOCK_HANDLE bindingLock)
{
    return Unlock(bindingLock);
}

LOCK_RESULT DT_LockBinding_LockDeinit_Impl(LOCK_HANDLE bindingLock)
{
    return Lock_Deinit(bindingLock);
}

void DT_ThreadBinding_ThreadSleep_Impl(unsigned int milliseconds)
{
    ThreadAPI_Sleep(milliseconds);
}

