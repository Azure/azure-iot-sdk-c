# Provisioning Service Client Sample for Query
## Overview

This is a quick tutorial with the steps to query Individual Enrollmens from the Microsoft Azure IoT Hub Device Provisioning Service using the C SDK.

## References

[Source Code][source-code-link]

## How to run the sample on Linux or Windows

1. Clone the [C SDK repository][root-link]
2. Compile the C SDK as shown [here][devbox-setup-link], using the `-Duse_prov_client=ON` flag.
3. Edit `prov_sc_bulk_query_sample.c` to add your provisioning service information:
    1. Replace the `[Connection String]` with the Provisioning Connection String copied from your Device Provisiong Service on the Portal.
        ```c
        const char* connectionString = "[Connection String]";
        ```
    2. This sample assumes there are already Individual Enrollments on your Device Provisioning Service. See the [prov_sc_individual_enrollment_sample][ie-sample-link] for instructions on how to create some.

    3. This sample uses Individual Enrollments, however query supports Enrollment Groups and Device Registration Statuses as well.

        1. In order to use Enrollment Groups:
            1. Use the `prov_sc_query_enrollment_group` function instead of `prov_sc_query_individual_enrollment`. 
                ```c
                prov_sc_query_enrollment_group(prov_sc, &query_spec, &cont_token, &query_resp);
                ```

            2. When accessing results, use the `eg` part of the `response_arr` union type instead of `ie`, where the elements will be of type `ENROLLMENT_GROUP_HANDLE`:
                ```c
                ENROLLMENT_GROUP_HANDLE eg = query_resp->response_arr.eg[i];
                ```

        2. In order to use Device Registration Statuses:
            1. Instead of setting the `query_spec.query_string` field, set the `query_spec.registration_id` field to the registration ID of the Enrollment Group you would like to query for Device Registration Statuses.
                ```c
                query_spec.registration_id = "[Registration Id]";
                ```

            2. Use the `prov_sc_query_device_registration_status` function instead of `prov_sc_query_individual_enrollment`. 
                ```c
                prov_sc_query_device_registration_state(prov_sc, &query_spec, &cont_token, &query_resp)
                ```
            
            3. When accessing results, use the `drs` part of the `response_arr` union type instead of `ie`, where the elements will be of type `DEVICE_REGISTRATION_STATE_HANDLE`:
                ```c
                DEVICE_REGISTRATION_STATE_HANDLE drs = query_resp->response_arr.drs[i];
                ```

4. Build as shown [here][devbox-setup-link] and run the sample.

[ie-sample-link]:../prov_sc_individual_enrollment_sample
[root-link]: https://github.com/Azure/azure-iot-sdk-c
[source-code-link]: ../../src
[devbox-setup-link]: ../../../doc/devbox_setup.md