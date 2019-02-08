# Azure Iot Provisioning Client FAQ

Here is where we answer question pertaining to the IoT Provisioning Client.  Please read the following documents for an overview of the [Device Provisioning Service](https://docs.microsoft.com/en-us/azure/iot-dps/)

## x509 Provisioning

- How does the provisioning service match the device's X.509 certificate to an enrollment?

  - The Provisioning Client adds the x509 certificate on the TLS stream.  The Device Provisioning Service uses this certificate to look up the proper enrollment record.

- Where does the x509 certificate come from?

  - This question depends on the situation:

    - **Development**: The SDK ships with a development repo that generates test x509 with a pre-generated key.  This enables the developer to quickly get up and running to test their solutions.

    - **Production**: For productions situations the developer should create a custom HSM library to retrieve the certificate from a hardware backed HSM or a software solution (for more information on this please see [using custom hsm](https://github.com/Azure/azure-iot-sdk-c/blob/master/provisioning_client/devdoc/using_custom_hsm.md))

    For more reading on certificates information please see [security concept document](https://docs.microsoft.com/en-us/azure/iot-dps/concepts-security#x509-certificates)

- What if I want the certificate private key to never leave the hardware?

  - One of the benefits of having an HSM is that it can ensure that the private key will never leave the hardware device, but to get this functionality it will require a little more code.

    - You will need to write a custom HSM to be able to extract the certificate and return the alias private key to the SDK (more on this in a moment) see the [custom hsm sample](https://github.com/Azure/azure-iot-sdk-c/blob/master/provisioning_client/samples/custom_hsm_example/custom_hsm_example.c) in the SDK.

    - You will need to have a TLS engine that can communicate with the target hardware that is connected to the device.  You can review your hardware device documentation for information on obtaining a TLS engine.

    - You will also need to create a custom azure iot tlsio library to communicate with the hardware tls engine.  You can use the [tlsio_template](https://github.com/Azure/azure-c-shared-utility/blob/master/adapters/tlsio_template.c) to get started or you can look at an already complete tlsio such as [tlsio_openssl](https://github.com/Azure/azure-c-shared-utility/blob/master/adapters/tlsio_openssl.c)

  - More on Alias Private Keys

    - The custom HSM uses the concept of **Alias Private Keys** to deal with keeping the private keys information in the hardware.  The developer can pass a token that can be interpreted by the hardware stack and the SDK will simply pass the token down to the custom tlsio interface without inspecting the value of the token.  This way the actual keys never leave the hardware.

## Symmetric Key Provisioning

- Coming soon

For more reading on Symmetric key information please see [Symmetric key concept doc](https://docs.microsoft.com/en-us/azure/iot-dps/concepts-symmetric-key-attestation)

## TPM Provisioning

- Coming soon

For more reading on TPM information please see [TPM Attestation doc](https://docs.microsoft.com/en-us/azure/iot-dps/concepts-tpm-attestation)
