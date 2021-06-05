## Sample Trusted Root Certificate Store

The Azure IoT Hub certificates presented during TLS negotiation shall be always validated using the appropriate root CA certificate(s). The default TLS stacks used on Windows (SChannel), Linux (OpenSSL) and iOS (Secure Transport) will use Trusted Root Certificate Authority stores maintained by the respective operating system. 

At the time of writing, mbedTLS, WolfSSL and BearSSL do not support operating system's certificate stores. By choosing any of these TLS stacks, the application developer becomes responsible with creating and maintaining a Trusted Root Certificate Store. When using any of these TLS stacks, the samples in this repository can be configured to leverage the sample certificate store in `certs.c`. By default, all certs are included in the samples' build. To reduce binary size, select certificates only for Azure clouds that the device connects to. Use one of the following options during the cmake step of your [environment setup](https://github.com/Azure/azure-iot-sdk-c/doc/devbox_setup.md).

```
cmake .. -Duse_sample_trusted_cert                               // To trust all Azure Roots for samples.

cmake .. -Duse_sample_trusted_cert -Duse_azure_cloud_rsa_cert    // To trust only the Azure Cloud RSA Roots for samples.

cmake .. -Duse_sample_trusted_cert -Duse_azure_cloud_ecc_cert    // To trust only the Azure Cloud ECC Roots for samples.

cmake .. -Duse_sample_trusted_cert -Duse_microsoftazure_de_cert  // To trust only the Azure Germany Cloud Root for samples.

cmake .. -Duse_sample_trusted_cert -Duse_portal_azure_cn_cert    // To trust only the Azure China Cloud Root for samples.
```

For other regions (and private cloud environments), please use the appropriate root CA certificate.

__IMPORTANT:__

1. The content of this repository is not guaranteed to be up to date. It is also not guaranteed to be a complete list of CA certificates used across Azure Clouds or when using Azure Stack, Azure IoT Edge, or Azure Protocol Gateway.
1. Always prefer using the local system's Trusted Root Certificate Authority store instead of hardcoded certificates (e.g., certs.c, which our samples require in certain combinations).
1. Azure Root certificates may change with or without prior notice (e.g., if they expire or are revoked). It is important that devices are able to add or remove trust in root certificates.
1. Support for at least two certificates is required to maintain device connectivity during CA certificate changes.

A couple of examples:

- Windows: Schannel will automatically pick up CA certificates from the store managed using `certmgr.msc`.
- Debian Linux: OpenSSL will automatically pick up CA certificates from the store installed using `apt install ca-certificates`. Adding a certificate to the store is described here: http://manpages.ubuntu.com/manpages/precise/man8/update-ca-certificates.8.html

## Additional Information

For additional guidance and important information about certificates, please refer to [this blog post](https://techcommunity.microsoft.com/t5/internet-of-things/azure-iot-tls-critical-changes-are-almost-here-and-why-you/ba-p/2393169) from the security team.

## IoT Hub ECC Server Certificate Chain

To work with the new Azure Cloud ECC server certificate chain, the TLS stack must be configured to prevent RSA cipher-suites from being advertised as described [here](https://docs.microsoft.com/azure/iot-hub/iot-hub-tls-support#elliptic-curve-cryptography-ecc-server-tls-certificate-preview).

### Using OpenSSL

Use the following option to limit the advertised cipher-suites:
```C
IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_OPENSSL_CIPHER_SUITE, "ECDH+ECDSA+HIGH");
```

### Using wolfSSL

Rebuild wolfSSL without RSA support by adding `--disable-rsa` to the `configure` step.

### Using mbedTLS

Change `include/mbedtls/config.h` by commenting out all `_RSA_ENABLED` defines:
```C
MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED
MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED
```

Also, ensure you have the following two options enabled for ECC P-384 and SHA384 support:
```C
MBEDTLS_ECP_DP_SECP384R1_ENABLED
MBEDTLS_SHA512_C
```
