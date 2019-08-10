// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LOCK_THREAD_BINDING__H
#define LOCK_THREAD_BINDING__H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "azure_c_shared_utility/lock.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef LOCK_HANDLE(*DT_BINDING_LOCK_INIT)(void);
typedef LOCK_RESULT(*DT_BINDING_LOCK)(LOCK_HANDLE bindingLock);
typedef LOCK_RESULT(*DT_BINDING_UNLOCK)(LOCK_HANDLE bindingLock);
typedef LOCK_RESULT(*DT_BINDING_LOCK_DEINIT)(LOCK_HANDLE bindingLock);
typedef void(*DT_BINDING_THREAD_SLEEP)(unsigned int milliseconds);

typedef struct DT_LOCK_THREAD_BINDING_TAG
{
    // Lock and callbacks for interacting with the binding lock logic.  For _LL_ layer, these are no-ops.        
    LOCK_HANDLE dtBindingLockHandle;
    DT_BINDING_LOCK_INIT dtBindingLockInit;
    DT_BINDING_LOCK dtBindingLock;
    DT_BINDING_UNLOCK dtBindingUnlock;
    DT_BINDING_LOCK_DEINIT dtBindingLockDeinit;
    DT_BINDING_THREAD_SLEEP dtBindingThreadSleep;
} DT_LOCK_THREAD_BINDING;

#ifdef __cplusplus
}
#endif

#endif // LOCK_THREAD_BINDING__H

