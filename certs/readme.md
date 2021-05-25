## Certificates -  Important to know

The Azure IoT Hub certificates presented during TLS negotiation shall be always validated using the appropriate root CA certificate(s).

The samples in this repository leverage the certificates in `certs.c` for the Azure Cloud, Germany sovereign cloud and China sovereign cloud. By default, all certs are included in the build. To select a specific cert, use one of the following options during the cmake step of your [environment setup](https://github.com/Azure/azure-iot-sdk-c/doc/devbox_setup.md).

```
cmake .. -Duse_sample_trusted_cert                               // To trust all Azure Roots.

cmake .. -Duse_sample_trusted_cert -Duse_azure_cloud_rsa_cert    // To trust the Azure Cloud RSA Roots only.

cmake .. -Duse_sample_trusted_cert -Duse_azure_cloud_ecc_cert    // To trust the Azure Cloud ECC Roots only.

cmake .. -Duse_sample_trusted_cert -Duse_microsoftazure_de_cert  // To trust Azure Germany Cloud Root only.

cmake .. -Duse_sample_trusted_cert -Duse_portal_azure_cn_cert    // To trust Azure China Cloud Root only.
```

For other regions (and private cloud environments), please use the appropriate root CA certificate.

__IMPORTANT:__

1. The content of this repository is not guaranteed to be up to date or a complete list of CA certificates used across Azure Clouds or when using Azure Stack or Azure IoT Edge or Azure Protocol Gateway.
1. Always prefer using the local system's Trusted Root Certificate Authority store instead of hardcoded certificates (i.e. using certs.c which our samples require in certain combinations).
1. Azure Root certificates may change, with or without prior notice (if they expire or are revoked). It is important that devices are able to add or remove trust in root certificates.
1. Support for at least 2 certificates is required to maintain device connectivity during CA certificate changes.

A couple of examples:

- Windows: Schannel will automatically pick up CA certificates from the store managed using `certmgr.msc`.
- Debian Linux: OpenSSL will automatically pick up CA certificates from the store installed using `apt install ca-certificates`. Adding a certificate to the store is described here: http://manpages.ubuntu.com/manpages/precise/man8/update-ca-certificates.8.html

## Additional Information

For additional guidance and important information about certificates, please refer to [this blog post](https://techcommunity.microsoft.com/t5/internet-of-things/azure-iot-tls-changes-are-coming-and-why-you-should-care/ba-p/1658456) from the security team.
