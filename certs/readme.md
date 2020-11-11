## Certificates - Important to know

For sample purposes, C SDK samples leverage certificates on the `certs.c` file. When creating solutions based on the C SDK you should consider :

1. For production environments, the IoT Hub certificate must be validated during the TLS handshake.
2. We use the Baltimore root CA to validate IoT Hub and Device Provisioning Service (DPS) server certificate (for the regions that use this certificate)
3. For other regions (and private cloud environments) the appropriate root CA shall be used

## Additional Information

For additional guidance and important information about certificates, please refer to [this blog post](https://techcommunity.microsoft.com/t5/internet-of-things/azure-iot-tls-changes-are-coming-and-why-you-should-care/ba-p/1658456) from the security team. 

