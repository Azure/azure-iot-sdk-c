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
#include "umock_c/umock_c.h"


static const int DT_TEST_MILLISECONDS = 0x1234;

#include "internal/dt_lock_thread_binding_stub.h"

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}
MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(LOCK_RESULT, LOCK_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(LOCK_RESULT, LOCK_RESULT_VALUES);

BEGIN_TEST_SUITE(dt_lock_thread_binding_stub_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LOCK_RESULT, int);
}

TEST_FUNCTION_INITIALIZE(TestMethodInit)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    ;
}

TEST_FUNCTION(DT_LockBinding_LockInit_Stub_ok)
{
    //act
    LOCK_HANDLE h = DT_LockBinding_LockInit_Stub();

    //assert
    ASSERT_IS_NOT_NULL(h);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_LockBinding_Lock_Stub_ok)
{
    //act
    LOCK_RESULT lockResult = DT_LockBinding_Lock_Stub(NULL);

    //assert
    ASSERT_ARE_EQUAL(LOCK_RESULT, lockResult, LOCK_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_LockBinding_Unlock_Stub_ok)
{
    //act
    LOCK_RESULT lockResult = DT_LockBinding_Unlock_Stub(NULL);

    //assert
    ASSERT_ARE_EQUAL(LOCK_RESULT, lockResult, LOCK_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_LockBinding_LockDeinit_Stub_ok)
{
    //act
    LOCK_RESULT lockResult = DT_LockBinding_LockDeinit_Stub(NULL);

    //assert
    ASSERT_ARE_EQUAL(LOCK_RESULT, lockResult, LOCK_OK);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

TEST_FUNCTION(DT_ThreadBinding_ThreadSleep_Stub_ok)
{
    //act
    DT_ThreadBinding_ThreadSleep_Stub(DT_TEST_MILLISECONDS);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(dt_lock_thread_binding_stub_ut)

