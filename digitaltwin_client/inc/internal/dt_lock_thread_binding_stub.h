// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LOCK_THREAD_BINDING_STUB_H
#define LOCK_THREAD_BINDING_STUB_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/threadapi.h"

#ifdef __cplusplus
extern "C"
{
#endif

MOCKABLE_FUNCTION(, LOCK_HANDLE, DT_LockBinding_LockInit_Stub);
MOCKABLE_FUNCTION(, LOCK_RESULT, DT_LockBinding_Lock_Stub, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, LOCK_RESULT, DT_LockBinding_Unlock_Stub, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, LOCK_RESULT, DT_LockBinding_LockDeinit_Stub, LOCK_HANDLE, bindingLock);
MOCKABLE_FUNCTION(, void, DT_ThreadBinding_ThreadSleep_Stub, unsigned int, milliseconds);

#ifdef __cplusplus
}
#endif


#endif // LOCK_THREAD_BINDING_STUB_H

