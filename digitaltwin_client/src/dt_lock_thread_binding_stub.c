// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "internal/dt_lock_thread_binding_stub.h"

static const LOCK_HANDLE stubLockHandle = (LOCK_HANDLE)0x1234;

LOCK_HANDLE DT_LockBinding_LockInit_Stub(void)
{
    return stubLockHandle;
}

LOCK_RESULT DT_LockBinding_Lock_Stub(LOCK_HANDLE bindingLock)
{
    (void)bindingLock;
    return LOCK_OK;
}

LOCK_RESULT DT_LockBinding_Unlock_Stub(LOCK_HANDLE bindingLock)
{
    (void)bindingLock;
    return LOCK_OK;
}

LOCK_RESULT DT_LockBinding_LockDeinit_Stub(LOCK_HANDLE bindingLock)
{
    (void)bindingLock;
    return LOCK_OK;
}

void DT_ThreadBinding_ThreadSleep_Stub(unsigned int milliseconds)
{
    (void)milliseconds;
}

