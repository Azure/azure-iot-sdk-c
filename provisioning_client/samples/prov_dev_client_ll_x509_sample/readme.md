# Azure IoT Provisioning Client Sample: Using X509 Client Certificates

## Build instructions

1. Modify `.\prov_dev_client_ll_x509_sample.c`: update the `id_scope`, `registration_id`, 
   `x509certificate` and `x509privatekey` variables.
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

### TLS Options

Additional TLS stack specific configuration is available through the following two APIs:

* For DPS: `Prov_Device_LL_SetOption`
* For Hub: `IoTHubDeviceClient_LL_SetOption`

Some of the available options can be found [here](https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/Iothub_sdk_options.md#common-transport-options).

Both DPS and Hub clients should be configured with the same TLS stack parameters. (e.g. if configuring 
OPTION_OPENSSL_ENGINE by calling `Prov_Device_LL_SetOption`, the application must also call 
`IoTHubDeviceClient_LL_SetOption` using the same OPTION_OPENSSL_ENGINE key and the same engine name.)

### Long X509 Client Certificate Chains

When using chains with more than one level, the device must send all chain certificates up to but 
not including the CA configured on the service. The certificates will be sent as part of the TLS 
handshake in order for the service to build a valid chain. 

For TLS stacks such as OpenSSL and mbedTLS, this is achieved by creating a PEM certificate store 
(i.e. concatenating the Intermediates as well as the Device Certificate PEM contents) and passing 
it to the C-SDK via the `x509certificate` variable.

In Windows, all intermediates must be present in the Intermediate Certificate Store and the
`x509certificate` variable should contain only the device certificate. SChannel will automatically 
build the chain and send the required certificates to the service.

__Note:__ Using a multi-level chain increases the overhead of each TLS connection. All intermediate 
certificates unknown to the service must be sent on every new connection. Both the client and 
service TLS stacks have limitations on the TLS record lengths. Long chains (or chains containing 
several large certificates) may not work.
