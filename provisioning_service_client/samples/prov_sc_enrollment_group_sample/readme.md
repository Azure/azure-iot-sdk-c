# Provisioning Service Client Sample for Enrollment Group

## Overview

This is a quick tutorial with the steps to create, update, get, and delete an Enrollment Group on the Microsoft Azure IoT Hub Device Provisioning Service using the C SDK.

## References

[Source Code][source-code-link]

## How to run the sample on Linux or Windows

1. Clone the [C SDK repository][root-link]
2. Compile the C SDK as shown [here][devbox-setup-link], using the `-Duse_prov_client=ON` flag.
3. Edit `provisioning_enrollment_group_sample.c` to add your provisioning service information:
    1. Replace the `[Connection String]` with the Provisioning Connection String copied from your Device Provisiong Service on the Portal.
        ```c
        const char* connectionString = "[Connection String]";
        ```
    2. Replace the `[Group Id]` with your chosen Group Id.
    3. For using a **Signing Certificate** (as shown in the sample):
        1. Replace the `[Signing Certificate]` with your signing certificate. You can use a physical device with [DICE][dice-link], or use a certificate you generate yourself. One possible way to do this is to use the included [CA Certificates Tool][ca-cert-link].
            ```c
            const char* signingCertificate = "[Signing Certificate]";
            ```
            Note that a certificate format can be just the Base 64 encoding, or can include the `-----BEGIN CERTIFICATE-----` and `-----END CERTIFICATE-----` tags, either works.
    4. For using a **CA Certificate Reference** (not shown in this sample):
        1. Define a variable `caReference`, and set it to the name of the CA Certificate you have [generated on the Portal][ca-cert-portal-link].
            ```c
            const char* caReference = "[CA Reference]"
            ```
        2. Replace the Signing Certificate x509 Attestation Mechanism in the sample with a CA Reference x509 Attestation Mechanism

            **Replace**
            ```c
            if ((am_handle = attestationMechanism_createWithX509SigningCert(signingCertificate, NULL)) == NULL)
            {
                printf("Failed calling attestationMechanism_createX509SigningCert\n");
                result = MU_FAILURE;
            }
            ```

            **With**
            ```c
            if ((am_handle = attestationMechanism_createWithX509CAReference(caReference, NULL)) == NULL)
            {
                printf("Failed calling attestationMechansim_createX509CAReference\n");
                result = MU_FAILURE;
            }
            ```


4. Build as shown [here][devbox-setup-link] and run the sample.

[root-link]: https://github.com/Azure/azure-iot-sdk-c
[source-code-link]: ../../src
[dice-link]: https://azure.microsoft.com/blog/azure-iot-supports-new-security-hardware-to-strengthen-iot-security/
[devbox-setup-link]: ../../../doc/devbox-setup.md
[ca-cert-link]: ../../../tools/CACertificates
[ca-cert-portal-link]: https://docs.microsoft.com/azure/iot-hub/iot-hub-security-x509-get-started