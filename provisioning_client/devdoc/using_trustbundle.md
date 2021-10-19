# Provisioning CA Certificates to Devices using TrustBundle

In general, local services or Azure IoT Edge devices do not have server certificates signed by 
authorities trusted by the device. To allow the device to connect securely (for example to an Azure 
IoT Edge), new certificates must be installed within the Trusted Root Certification Authority (CA) 
store of the device. 

The TrustBundle Device Provisioning Service feature allows developers to manage the device CA 
certificate store remotely.

Also see https://azure.github.io/iot-identity-service/

## Application Architecture

For security reasons, it is recommended to install certificates using a separate, elevated privilege
process instead of granting more permissions to the Provisioning or Hub application components:

![trust_bundle_application](https://www.plantuml.com/plantuml/png/XPF1Ri8m38RlUGghf-rG7c124mYGXiO13NQQTWYjAH4XHabQf4sy-vpGGQkrx5RP_lpRlzEHyzBwyg35rie3GhAW4oojgfJ60XFu5W0VIqkLSegCCWLCw70aWyP_XjHBkMb6pa8SPRQN1NUQQQoanxpHBdHZQ8BMgwtAE0jpmnDeZJR2cQOoluXEiL8PGajxXJO4e_ASri3g4HEvz78Z7QkkRUc2Q5DZvSdMkp0u_Yejwp8-6SCRKLo4uxESfsw75fH9_QjwovsRWftBm9JpLqKjdOSARLW37j3Buh4Mq8epj0LLWpbajtOkAjqr0ePfsdkUgqMXwi-f-Z18q-VU4_N4BqpRmBkbFSRsCSF0TBfud_Z7NM68AQkANInBkXr9dc2Xp9xfa_8xy3iUE5rDNo-qfsDaM-ucujsXYwNrzNgVvK1qDXy8D3a41I56_Cb_w0y0 "trust_bundle_application")


The Certificate Installer component must be able to store the last TrustBundle version (etag) as 
well as the list of installed certificates. When a new version of the TrustBundle is received, 
Certificate Installer must first remove certificates that are no longer part of the TrustBundle then
install new certificates from the TrustBundle. 

__Important:__ Accidentally deleting the Azure IoT CA root on the device will result in permanent Azure IoT service 
connectivity loss.

## Public API

```C
// LL layer:
const char* Prov_Device_LL_Get_Trust_Bundle(PROV_DEVICE_LL_HANDLE handle)

// Convenience Layer:
const char* Prov_Device_Get_Trust_Bundle(PROV_DEVICE_HANDLE handle)
```

## Sample

The `prov_dev_client_ll_sample` demonstrates how to use the public API as well as parse the 
TrustBundle to extract the PEM certificates. Device Provisioning Service configuration is required
to enable and configure a TrustBundle for the individual or group device enrollments.

### Example output

```
Current trustbundle version: "5301e830-0000-0300-0000-613864190000"
TrustBundle has 2 CA certificates:
        Subject: CN=CA_ROOT1, OU=test, O=testloaner, C=16148
        Issuer : CN=CA_ROOT1, OU=test, O=testloaner, C=16148
        CA Root: yes
        PEM data:
-----BEGIN CERTIFICATE-----
MIIB6zCCAZKgAwIBAgIIZiChmKQYrUUwCgYIKoZIzj0EAwIwcDEyMDAGA1UEAwwpcm9vdC02YTQ2
...
q4AVzwgCIAgxApsBGYwslDboPToMR8zTkG8mTdd3I/MNf8qQBKfm
-----END CERTIFICATE-----

        Subject: CN=CA_ROOT2, OU=test, O=testloaner, C=16148
        Issuer : CN=CA_ROOT2, OU=test, O=testloaner, C=16148
        CA Root: yes
        PEM data:
-----BEGIN CERTIFICATE-----
...
