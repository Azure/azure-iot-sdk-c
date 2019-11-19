// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"
#include "digitaltwin_client_version.h"

BEGIN_TEST_SUITE(dt_version_ut)

    TEST_FUNCTION(DigitalTwin_Client_GetVersionString_ok)
    {
        ASSERT_ARE_EQUAL(char_ptr, "0.9.0", DigitalTwin_Client_GetVersionString());
    }

END_TEST_SUITE(dt_version_ut)

