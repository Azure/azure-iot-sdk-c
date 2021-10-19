# Using Short-Lived Device Certificates

Device Provisioning Service can be configured to provision short lived, operational X509 
certificates to IoT devices. These operational certificates must be used to authenticate with the 
allocated IoT Hub. A bootstrap authentication mechanism (either SAS token or X509 Client 
Certificate) is still required to authenticate the device with DPS.

## Certificate Signing Request Flow

![dpsCSR](https://www.plantuml.com/plantuml/png/bLF1JW8n4BtlLynH2RinCHuaXWYBg34OOJ4nCHxATe29jLtRBihVExiLWxX6kRNJUM_UcvcUEo-iBryKoCAbsIGQu8foXBWBuTI1IzHeXKTejKnHdSXeeLejk4XJUCPrN0Yo3RZK8gCSf6WzpIclA39QQD8BcE1hYSv3wQhRBd4JwLtMWLxfbwXze0hGh9UrONet0cCXLSIly71oTCgKCsEyyrOKJ9XRb1LGA1SnqxRANdgpMyRYWfn72mVX59HTopRqZLntViZbjXqsqeRGbl_AWB7acqbgm5dyJq1jSojrrmKtsAxXCKcIfrrnot8s70zkso3hGENCSHaVArgXtAVIrNs_SInxqJ59tAsPFed_GW2-5yGZJJPAk6arVZIUJc50BZTQO-uZRJXPuTn74DwEGHfUIOu3vtX16lX9I4cX6CWlCA-1S4Od4MfP0HfjDzZzhLiRZLlTuf8m5AHAYz-aJfVaW3Um01Q2pWaUfD5gNMbzO_1drU2eKU1cqXEi_pVCbNcBedFEvA_-0G00 "dpsCSR")

## Public API

```C
// LL layer:
PROV_DEVICE_RESULT Prov_Device_LL_Set_Certificate_Signing_Request(PROV_DEVICE_LL_HANDLE handle, const char* csr)
const char* Prov_Device_LL_Get_Issued_Client_Certificate(PROV_DEVICE_LL_HANDLE handle)

// Convenience Layer:
PROV_DEVICE_RESULT Prov_Device_Set_Certificate_Signing_Request(PROV_DEVICE_HANDLE handle, const char* csr)
const char* Prov_Device_Get_Issued_Client_Certificate(PROV_DEVICE_HANDLE handle)
```

## Sample

The `prov_dev_client_ll_csr_sample` demonstrates how to use the public API to submit a certificate 
signing request then use the provisioned device certificate when connecting to Azure IoT Hub.

### Using OpenSSL

1. Generate a key-pair

```
# Generate an ECC P-256 keypair:
openssl ecparam -out key.pem -name prime256v1 -genkey

# Generate an RSA 4096 bit keypair:
openssl genrsa -out key.pem 4096
```

2. Generate a PKCS#10 Certificate Signing Request from the keypair:

Replace `my-registration-id` with the device registration ID.

_Important:_ DPS has character set restrictions: https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#registration-id

```
openssl req -new -key key.pem -out request.csr -subj '/CN=my-registration-id'
```

To read the information from a CSR file use:

```
openssl req -noout -text -in request.csr
```

For more information see https://www.openssl.org/docs/man1.1.1/man1/openssl-req.html

3. Modify the CSR sample by populating the `x509csr` and `x509privatekey` variables with the content
of the `request.csr` and `key.pem` files respectively.

### Using mbedTLS

See https://tls.mbed.org/kb/how-to/generate-a-certificate-request-csr

### Using WolfSSL

See https://www.wolfssl.com/certificate-signing-request-csr-generation-wolfssl

### Using Windows / SChannel

See https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/certreq_1
