// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#endif

#include "testrunnerswitcher.h"


#define ENABLE_MOCKS
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/threadapi.h"
#undef ENABLE_MOCKS

static const LOCK_HANDLE DT_TEST_LOCK_HANDLE = (LOCK_HANDLE)0x4244;
static const int DT_TEST_MILLISECONDS = 0x1234;


#include "internal/dt_lock_thread_binding_impl.h"

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}
MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(LOCK_RESULT, LOCK_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(LOCK_RESULT, LOCK_RESULT_VALUES);

BEGIN_TEST_SUITE(dt_lock_thread_binding_impl_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_RESULT, int);

    REGISTER_GLOBAL_MOCK_RETURN(Lock_Init, DT_TEST_LOCK_HANDLE);
    REGISTER_GLOBAL_MOCK_RETURN(Lock, LOCK_OK);
    REGISTER_GLOBAL_MOCK_RETURN(Unlock, LOCK_OK);
}

TEST_FUNCTION_INITIALIZE(TestMethodInit)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    ;
}

TEST_FUNCTION(DT_LockBinding_LockInit_Impl_ok)
{
    //arrange
    STRICT_EXPECTED_CALL(Lock_Init());

    //act
    LOCK_HANDLE h = DT_LockBinding_LockInit_Impl();

    //assert
    ASSERT_ARE_EQUAL(void_ptr, h, DT_TEST_LOCK_HANDLE);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_LockBinding_Lock_Impl_ok)
{
    //arrange
    STRICT_EXPECTED_CALL(Lock(DT_TEST_LOCK_HANDLE));

    //act
    LOCK_RESULT lockResult = DT_LockBinding_Lock_Impl(DT_TEST_LOCK_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(LOCK_RESULT, lockResult, LOCK_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_LockBinding_Unlock_Impl_ok)
{
    //arrange
    STRICT_EXPECTED_CALL(Unlock(DT_TEST_LOCK_HANDLE));

    //act
    LOCK_RESULT lockResult = DT_LockBinding_Unlock_Impl(DT_TEST_LOCK_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(LOCK_RESULT, lockResult, LOCK_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_LockBinding_LockDeinit_Impl_ok)
{
    //arrange
    STRICT_EXPECTED_CALL(Lock_Deinit(DT_TEST_LOCK_HANDLE));

    //act
    LOCK_RESULT lockResult = DT_LockBinding_LockDeinit_Impl(DT_TEST_LOCK_HANDLE);

    //assert
    ASSERT_ARE_EQUAL(LOCK_RESULT, lockResult, LOCK_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_ThreadBinding_ThreadSleep_Impl_ok)
{
    //arrange
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(DT_TEST_MILLISECONDS));

    //act
    DT_ThreadBinding_ThreadSleep_Impl(DT_TEST_MILLISECONDS);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


END_TEST_SUITE(dt_lock_thread_binding_impl_ut)

