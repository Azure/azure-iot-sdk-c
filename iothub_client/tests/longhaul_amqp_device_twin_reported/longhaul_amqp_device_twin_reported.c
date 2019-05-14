// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransportamqp.h"
#include "iothub_account.h"
#include "iothubtest.h"
#include "../common_longhaul/iothub_client_common_longhaul.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#define INDEFINITE_TIME ((time_t)-1)

int main(void)
{
    int result;
    IOTHUB_LONGHAUL_RESOURCES_HANDLE iotHubLonghaulRsrcsHandle;
    size_t test_duration_in_seconds = 12 * 60 * 60;
    size_t test_loop_wait_time_in_seconds = 60;

    if ((iotHubLonghaulRsrcsHandle = longhaul_tests_init()) == NULL)
    {
        LogError("Test failed");
        result = MU_FAILURE;
    }
    else
    {
        if (longhaul_initialize_device_client(iotHubLonghaulRsrcsHandle, IoTHubAccount_GetSASDevice(longhaul_get_account_info(iotHubLonghaulRsrcsHandle)), AMQP_Protocol) == NULL)
        {
            LogError("Failed creating the device client");
            result = MU_FAILURE;
        }
        else
        {
            result = longhaul_run_twin_reported_properties_tests(iotHubLonghaulRsrcsHandle, test_loop_wait_time_in_seconds, test_duration_in_seconds);
        }

        longhaul_tests_deinit(iotHubLonghaulRsrcsHandle);
    }

    return result;
}
