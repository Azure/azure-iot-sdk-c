## Certificates - Important to know

For sample purposes, C SDK samples leverage certificates on the `certs.c` file. When creating solutions based on the C SDK you should consider :

1. For production environments, the IoT Hub certificate must be validated during the TLS handshake.
2. We use the Baltimore root CA to validate IoT Hub server certificate (for the regions that use this certificate)
3. For other regions (and private cloud environments) the appropriate root CA shall be used

## Certificate update strategy

As shared on [this blog post](https://techcommunity.microsoft.com/t5/internet-of-things/azure-iot-tls-changes-are-coming-and-why-you-should-care/ba-p/1658456), Microsoft began phasing out her older PKI provided by Baltimore Cybertrust Root this August in favor of new PKI providers. For Azure IoT, this means that Azure IoT services will serve TLS certificates that will be rooted to the following:

- [Baltimore CyberTrust Root](https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt): Although this root is expiring in 2025, IoT Hub and Device Provisioning Service (DPS) will remain chained to this root until 2023 and a plan with defined timelines will be announced in March, 2021. 

- [Digicert Global Root G2](https://cacerts.digicert.com/DigiCertGlobalRootG2.crt): This is the new root that most of public Azure is chained to including all Azure IoT Services except IoT Hub and DPS as per 1

- MS-PKI: Microsoft is eventually going to move public Azure to her own public PKI that has the following two roots:

  - [RSA](http://www.microsoft.com/pkiops/certs/Microsoft%20RSA%20Root%20Certificate%20Authority%202017.crt) - Microsoft RSA Root Certificate Authority 2017
  - [ECC](https://www.microsoft.com/pkiops/certs/Microsoft%20EV%20ECC%20Root%20Certificate%20Authority%202017.crt) - Microsoft EV ECC Root Certificate Authority 2017

In order for devices to remain connected to Azure, Microsoft recommends adding all these roots to your device/certificate store. If you have a memory constrained device then Baltimore root will be the certificate to use for TLS until 2023, after which the guidance that will be provided then can be used. 
