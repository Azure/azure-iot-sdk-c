# Provisioning Service Client Sample for Bulk Operation

## Overview

This is a quick tutorial with the steps to create and delete Individual Enrollments with Bulk Operations on the Microsoft Azure IoT Hub Device Provisioning Service using the C SDK.

## References

[Source Code][source-code-link]

## How to run the sample on Linux or Windows

1. Clone the [C SDK repository][root-link]
2. Compile the C SDK as shown [here][devbox-setup-link], using the `-Duse_prov_client=ON` flag.
3. Edit `prov_sc_bulk_operation_sample.c` to add your provisioning service information:
    1. Replace the `[Connection String]` with the Provisioning Connection String copied from your Device Provisiong Service on the Portal.
        ```c
        const char* connectionString = "[Connection String]";
        ```

    2. This sample uses TPM attestations. You can also use X509 Attestations, by following the instructions laid out in the Individual Enrollment    sample instead.

        1. You must use an endorsement key for a TPM attestation.  If you do not have a physical device with a TPM, you can create a Registration ID yourself, and use the following endorsement key for testing purposes:

            ```AToAAQALAAMAsgAgg3GXZ0SEs/gakMyNRqXXJP1S124GUgtk8qHaGzMUaaoABgCAAEMAEAgAAAAAAAEAxsj2gUScTk1UjuioeTlfGYZrrimExB+bScH75adUMRIi2UOMxG1kw4y+9RW/IVoMl4e620VxZad0ARX2gUqVjYO7KPVt3dyKhZS3dkcvfBisBhP1XH9B33VqHG9SHnbnQXdBUaCgKAfxome8UmBKfe+naTsE5fkvjb/do3/dD6l4sGBwFCnKRdln4XpM03zLpoHFao8zOwt8l/uP3qUIxmCYv9A7m69Ms+5/pCkTu/rK4mRDsfhZ0QLfbzVI6zQFOKF/rwsfBtFeWlWtcuJMKlXdD8TXWElTzgh7JS4qhFzreL0c1mI0GCj+Aws0usZh7dLIVPnlgZcBhgy1SSDQMQ==```

        2. Replace the `[Registration Id #1]` and `[Registration Id #2]` each with a unique Reigstration ID, and `[Endorsement Key]` with the Endorsement Key. Note that in this example for simplicity we will use the same endorsement key for both individual enrollments, but you can (and should) use different ones.
            ```c
            const char* endorsementKey = "[Endorsement Key]";
            const char* registrationId1 = "[Registration Id #1]";
            const char* registrationId2 = "[Registration Id #2]";
            ```

4. Build as shown [here][devbox-setup-link] and run the sample.

[root-link]: https://github.com/Azure/azure-iot-sdk-c
[source-code-link]: ../../src
[tpm-simulator-link]: https://github.com/Azure/azure-iot-sdk-java/tree/main/provisioning/provisioning-tools/tpm-simulator
[dice-link]: https://azure.microsoft.com/blog/azure-iot-supports-new-security-hardware-to-strengthen-iot-security/
[devbox-setup-link]: ../../../doc/devbox_setup.md
[ca-cert-link]: ../../../tools/CACertificates