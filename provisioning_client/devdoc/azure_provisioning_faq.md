# Azure IoT Provisioning Client FAQ

Here is where we answer question pertaining to the IoT Provisioning Client.  Please read the following documents for an overview of the [Device Provisioning Service](https://docs.microsoft.com/azure/iot-dps/)

## x509 Provisioning

To use X509 Client authentication, enable `hsm_type_x509` when configuring the SDK. Support for DICE HSM is enabled by configuring the SDK with `hsm_type_riot`. Only one of the `hsm_type_x509` and `hsm_type_riot` options can be selected.

- How does the provisioning service match the device's X.509 certificate to an enrollment?

  - The Provisioning Client adds the x509 certificate on the TLS stream.  The Device Provisioning Service uses this certificate to look up the proper enrollment record.

- Where does the x509 certificate come from?

  - This question depends on the situation:

    - **Development**: The SDK ships with a way to either specify a PEM certificate and key, or to use a development DICE HSM implementation that generates a test x509 certificate with a pre-generated key. This enables the developer to quickly get up and running to test their solutions.

    - **Production**: For production situations, the developer should create a custom HSM library to retrieve the certificate from a hardware backed HSM or a software solution (for more information on this please see [using custom hsm](https://github.com/Azure/azure-iot-sdk-c/blob/main/provisioning_client/devdoc/using_custom_hsm.md))

    For more information on switching between development and production scenarios, refer to [using provisioning client](https://github.com/Azure/azure-iot-sdk-c/blob/main/provisioning_client/devdoc/using_provisioning_client.md)
    
    For more reading on certificates information, please see [security concept document](https://docs.microsoft.com/azure/iot-dps/concepts-security#x509-certificates)

- What if I want the certificate private key to never leave the hardware?

  - One of the benefits of having an HSM is that it can ensure that the private key will never leave the hardware device, but to get this functionality it will require a little more code.

    - For TLS stacks that already have drivers for your HSM (e.g. OpenSSL + the PKCS#11 Engine), you can use a key identifier instead of the in-memory PEM key, similar to the [IoT Hub Device Client](../../iothub_client/devdoc/iothubclient_c_library.md#openssl-engine-examples).
    
    - For TLS stacks that require custom code to access HSMs:
      - You will need to write a custom HSM module to be able to extract the certificate and return the alias private key to the SDK (more on this in a moment). See the [custom hsm sample](https://github.com/Azure/azure-iot-sdk-c/blob/main/provisioning_client/samples/custom_hsm_example/custom_hsm_example.c) in the SDK.

      - You will need to have a TLS engine that can communicate with the target hardware that is connected to the device.  You can review your hardware device documentation for information on obtaining a hardware TLS engine.

      - You will also need to create a custom Azure IoT tlsio library to communicate with the hardware TLS engine. You can use the [tlsio_template](https://github.com/Azure/azure-c-shared-utility/blob/master/adapters/tlsio_template.c) to get started, or you can look at an already complete tlsio such as [tlsio_openssl](https://github.com/Azure/azure-c-shared-utility/blob/master/adapters/tlsio_openssl.c)

    - __More on Alias Private Keys__ The DICE custom HSM uses the concept of **Alias Private Keys** to deal with keeping the private keys information in the hardware. The developer can pass a token that can be interpreted by the hardware stack and the SDK will simply pass the token down to the custom tlsio interface without inspecting the value of the token. This way the actual keys never leave the hardware.

## Symmetric Key Provisioning

To use Symmetric Key Provisioning, configure the SDK with `hsm_type_symm_key` enabled.

For more reading on Symmetric key information please see [Symmetric key concept doc](https://docs.microsoft.com/azure/iot-dps/concepts-symmetric-key-attestation)

## TPM Provisioning

To use TPM provisioning, configure the SDK with `hsm_type_sastoken` enabled.

For more reading on TPM information please see [TPM Attestation doc](https://docs.microsoft.com/azure/iot-dps/concepts-tpm-attestation)

## Related documents

[Porting guide for Azure IoT C SDK](https://github.com/Azure/azure-c-shared-utility/blob/master/devdoc/porting_guide.md)