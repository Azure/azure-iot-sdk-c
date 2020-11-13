## Certificates -  Important to know

The Azure IoT Hub certificates presented during TLS negotiation shall be always validated using the appropriate root CA certificate(s).

The samples in this repository leverage the certificates in `certs.c` for the United States, Germany sovereign cloud and China sovereign cloud.

For other regions (and private cloud environments), please use the appropriate root CA certificate.

Always prefer using the local system's Trusted Root Certificate Authority store instead of hardcoding the certificates (i.e. using certs.c such as our samples require in certain combinations). 

A couple of examples:

- Windows: Schannel will automatically pick up CA certificates from the store managed using `certmgr.msc`.
- Debian Linux: OpenSSL will automatically pick up CA certificates from the store installed using `apt install ca-certificates`. Adding a certificate to the store is described here: http://manpages.ubuntu.com/manpages/precise/man8/update-ca-certificates.8.html


## Additional Information

For additional guidance and important information about certificates, please refer to [this blog post](https://techcommunity.microsoft.com/t5/internet-of-things/azure-iot-tls-changes-are-coming-and-why-you-should-care/ba-p/1658456) from the security team. 
