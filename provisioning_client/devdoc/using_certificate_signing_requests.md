# Using Azure IoT Device Provisioning Certificate Signing Requests

Device Provisioning Service can be configured to provision operational X509 
certificates to IoT devices. These operational certificates must be used to authenticate with the 
allocated IoT Hub. An onboarding authentication mechanism (either SAS token or X509 Client 
Certificate) is still required to authenticate the device with DPS.

## Certificate Signing Request Flow

![dpsCSR](https://www.plantuml.com/plantuml/png/bPFVJy8m4CVVzrVSeoumJOmF4aCOGzGO331CJ8mFPJsWSRQpxKJ-Uwyh69nBmBV-kC_tldVNzenbsfRlEV329Eaq6E2do13QNV2h3joYHCqiGXYgmgs4aYmFGxX9ahDf6iCRRje54xg1JJGIQI11RSL2P4uc5Kifv1Ac-56YiL0QjwkBDucEqmx4fLsXj5xAescSjc0s7e7IaEI2Rk7vylpAISgvOffJ42bc6haZMMu2ajgt6ISFzJmQby9Or73YLzxQFMz1N_5DvuzVwjrfewm_sck0gq1fOPj5Ak2wVIHGrRaNMg-2Egmty195qMlTtAgS3oU3nnRmwi1LzW_rkwT-uomEIX3OxbRqLkmG0VXL21fTjCjEpQduqMGsWu4mcP8ICnj8HS4vBcm0_ku2kAAtH-T0CPO92NJ5E1S-6V0VcCRDZ99HW98xeB7KOqki-Tph4Y4mP28lDVwoEri90_JQ2Y0pQ0oZeIcPRvpVDS7RpBwgHfExgKwn-j2moDKw27eKIN_x6m00 "dpsCSR")

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

### 1. Generate a CSR

#### Using OpenSSL

1. Generate either a ECC or RSA key-pair

```
# Generate an ECC P-256 keypair:
openssl ecparam -out key.pem -name prime256v1 -genkey

# Generate an RSA 4096 bit keypair:
openssl genrsa -out key.pem 4096
```

2. Generate a PKCS#10 Certificate Signing Request (CSR) from the keypair:

Below, replace `my-registration-id` with the device registration ID.

_Important:_ DPS has character set restrictions: https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#registration-id

```
openssl req -new -key key.pem -out request.csr -subj '/CN=my-registration-id'
```

Additionally, to read the information from a CSR file use:

```
openssl req -noout -text -in request.csr
```

For more information on OpenSSL CSR support, see https://www.openssl.org/docs/man1.1.1/man1/openssl-req.html

#### Using mbedTLS to generate a CSR

See https://tls.mbed.org/kb/how-to/generate-a-certificate-request-csr

#### Using WolfSSL 

See https://www.wolfssl.com/certificate-signing-request-csr-generation-wolfssl

#### Using Windows / SChannel

See https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/certreq_1

### 2. Modify and run the C SDK CSR sample

Populate the onboarding and Device Provisioning information. 
The sample is limited to SAS token authentication for onboarding credentials:

```C
static const char* global_prov_uri = "global.azure-devices-provisioning.net";
static const char* id_scope = "[ID Scope]";

// SAS token based authentication information (requires CMake option `hsm_type_symm_key`):
static const char* registration_id = "[registration-id]";
static const char* shared_access_key = "[shared_access_key]";
```

Populate the `x509csr` and `x509privatekey` variables with the content of the `request.csr` and 
`key.pem` files respectively.

Running the sample will provision the device using onboarding credentials then use the operational
certificate (and device-bound private key) to send telemetry to the assigned IoT Hub.
