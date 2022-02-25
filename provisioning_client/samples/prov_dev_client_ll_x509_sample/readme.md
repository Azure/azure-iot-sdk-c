# Azure IoT Provisioning Using X509 Client Certificates

## Build instructions

1. Modify `.\prov_dev_client_ll_x509_sample.c`: update `id_scope`, `registration_id`, 
   `x509certificate` and `x509privatekey`.
   (Optionally, you can change configuration regarding transport, global endpoint, etc.)

1. If you are using an OpenSSL engine, uncomment the `#define SAMPLE_OPENSSL_ENGINE` and set it to the
   OpenSSL Engine name. Instead of a PEM certificate, `x509privatekey` will contain the private key ID
   as expected by the OpenSSL engine. (e.g. in case of engine `pkcs11`, `x509privatekey` will contain
   a PKCS#11 URI such as `pkcs11:object=test-privkey;type=private?pin-value=1234`.)

1. Configure the IoT SDK (Hub and DPS clients) to use the custom X509 HSM.

    ```sh
    # At the root of the repository
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug -Duse_prov_client=ON -Dhsm_type_x509=ON ..
    cmake --build . -j
    ```

## Run the sample

```sh
# At the root of the repository
cd build
./provisioning_client/samples/prov_dev_client_ll_x509_sample/prov_dev_client_ll_x509_sample
```

## Configuring the TLS Stack

Additional TLS stack specific configuration is available through the following two APIs:

* For DPS: `Prov_Device_LL_SetOption`
* For Hub: `IoTHubDeviceClient_LL_SetOption`

Both DPS and Hub clients should be configured with the same TLS stack parameters. (e.g. if configuring 
OPTION_OPENSSL_ENGINE by calling `Prov_Device_LL_SetOption`, the application must also call 
`IoTHubDeviceClient_LL_SetOption` using the same OPTION_OPENSSL_ENGINE key and the same engine name.)
