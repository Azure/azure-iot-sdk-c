# Azure Iot Provisioning Client FAQ

Here is where we shall try to answer question pertaining to the IoT Provisioning Client.  Please read the following documents for an overview of the DPS service.

## x509 Provisioning

- How does Provisining match the x509 certificates to an enrollment?

  - The Provisioning Client add the x509 certificate (more on the certificate in a second) on the TLS stream.  The DPS service uses this certificate to look up the proper enrollment record.

- Where does the x509 certificate comes from?

  - This question depends on the situation:

    - **Development**: The SDK ships with a development repo that generates test x509 with a canned key.  This enables the developer to quickly get up and running to test their solutions.

    - **Production**: For productions situations the developer should create a custom HSM library to retrieve the certificate from a hardware backed HSM or a software solution (for more information on this please see [using custom hsm](https://github.com/Azure/azure-iot-sdk-c/blob/master/provisioning_client/devdoc/using_custom_hsm.md))

- What if I want the certificate private key to never leave the hardware?

  - One of the benifits of having an HSM is that it can ensure that the private key will never leave the hardware device, but to get this functionality it will require a little more code.

    - You will need to write a custom HSM to be able to extract the certificate and return the alias private key to the SDK (more on this in a moment) see the [custom hsm sample](https://github.com/Azure/azure-iot-sdk-c/blob/master/provisioning_client/samples/custom_hsm_example/custom_hsm_example.c) in the SDK.

    - You will need to have a tls engine that can communicate with the target hardware that is connected to the device.

    - You will also need to create a custom azure iot tlsio library to communicate with the hardware tls engine.  You can use the [tlsio_template](https://github.com/Azure/azure-c-shared-utility/blob/master/adapters/tlsio_template.c) to get started or you can look at an already complete tlsio such as [tlsio_openssl](https://github.com/Azure/azure-c-shared-utility/blob/master/adapters/tlsio_openssl.c)

  - More on Alias Private Keys

    - The custom HSM uses the concept of **Alias Private Keys** to deal with keeping the private keys information in the hardware.  The developer can pass a token that can be interpreted by the hardware stack and the SDK will simply pass the token down to the custom tlsio interface without inspecting the value of the token.  This way the actual keys never leave the hardware.

## Symmetric Key Provisioning

- Coming soon

## TPM Provisioning

- Coming soon
