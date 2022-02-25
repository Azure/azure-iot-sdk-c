# Configure TLS Protocol Version and Ciphers

## How to configure the Azure IoT C SDK TLS platforms to disable TLS 1.0 and TLS 1.1

### SChannel (Microsoft Windows)

To use exclusively TLS 1.2 in Microsoft Windows using SChannel, disable the support for TLS 1.0 and 1.1.

Be aware that this action might impact other applications in the systems that might not support higher versions of TLS.

To disable support in SChannel for TLS 1.0, follow this [documentation](https://docs.microsoft.com/windows-server/identity/ad-fs/operations/manage-ssl-protocols-in-ad-fs#enable-and-disable-tls-10).

To disable support in SChannel for TLS 1.1, follow this [documentation](https://docs.microsoft.com/windows-server/identity/ad-fs/operations/manage-ssl-protocols-in-ad-fs#enable-and-disable-tls-11).

For more information on TLS configuration please see the [following documentation](https://docs.microsoft.com/dotnet/framework/network-programming/tls). 


### OpenSSL

The latest Azure IoT C SDK uses [TLS 1.2 by default with OpenSSL](https://github.com/Azure/azure-c-shared-utility/blob/48f7a556865731f0e96c47eb5e9537361f24647c/adapters/tlsio_openssl.c#L1167).

The SDK creates the SSL context using the method related to the TLS version selected (1.2 by default).

According to the [OpenSSL documentation](https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_new.html) the method selected causes OpenSSL to use only the TLS version associated with that method.  


### WolfSSL

To use only TLS 1.2 or higher, WolfSSL library MUST be built without using “--enable-oldtls”.

Please check the [WolfSSL documentation](https://github.com/wolfSSL/wolfssl/wiki/Building-wolfSSL#25-build-options-configure-options) as this option is enabled by default on the binaries distributed by them.

In its adapter layer, Azure IoT C SDK does not directly set the TLS version that wolfSSL should use.
The result is that the wolfSSL library will use its default TLS version. 


### Apple iOS

TLS 1.2+ should already be the default for the latest versions of Apple iOS. 


### mbedTLS

mbedTLS can be configured to use only higher versions of TLS. 
That is achieved by calling  [mbedtls_ssl_conf_min_version](https://os.mbed.com/teams/sandbox/code/mbedtls/docs/bef26f687287/ssl_8h.html).

In its adapter layer, Azure IoT C SDK already [sets the minimum TLS version that mbedTLS should use](https://github.com/Azure/azure-c-shared-utility/blob/48f7a556865731f0e96c47eb5e9537361f24647c/adapters/tlsio_mbedtls.c#L481) to TLS 1.2.


### BearSSL/esp8266 Arduino

TLS version can be set on BearSSL by using [br_ssl_engine_set_versions](https://www.bearssl.org/gitweb/?p=BearSSL;a=blob;f=inc/bearssl_ssl.h;h=fee7b3c496086b96b5435674c130af2e740b1b88;hb=5f045c759957fdff8c85716e6af99e10901fdac0#l1114).

Currently the Azure IoT C SDK does not directly set the minimum version of TLS to be used by BearSSL.



## How to configure ciphers in the Azure IoT C SDK supported TLS platforms


### SChannel (Microsoft Windows)

To configure additional ciphers for SChannel please follow this [documentation](https://docs.microsoft.com/windows-server/identity/ad-fs/operations/manage-ssl-protocols-in-ad-fs#enabling-or-disabling-additional-cipher-suites).


### OpenSSL

The OpenSSL API function [SSL_CTX_set_cipher_list](https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_set_cipher_list.html) can be used to set the list of ciphers used by the OpenSSL library.  

In the current Azure IoT C SDK code the [OpenSSL adapter already calls SSL_CTX_set_cipher_list](https://github.com/Azure/azure-c-shared-utility/blob/48f7a556865731f0e96c47eb5e9537361f24647c/adapters/tlsio_openssl.c#L966).

To take advantage of that functionality please use:

[IoTHubDeviceClient_LL_SetOption](https://github.com/Azure/azure-iot-sdk-c/blob/2019-12-11/iothub_client/inc/iothub_device_client_ll.h#L288), if using "iothub_device_client_ll.h", or;
  
  ```c
  #include "azure_c_shared_utility/shared_util_options.h" // for OPTION_OPENSSL_CIPHER_SUITE

  ...
  const char* ciphers = "list of ciphers, in the format expected by OpenSSL API's SSL_CTX_set_cipher_list()"; // please modify this string to have the desired ciphers.
  IoTHubDeviceClient_LL_SetOption(iotHubClientHandle, OPTION_OPENSSL_CIPHER_SUITE, ciphers);
  ```

[IoTHubDeviceClient_SetOption](https://github.com/Azure/azure-iot-sdk-c/blob/2019-12-11/iothub_client/inc/iothub_device_client.h#L275), if using "iothub_device_client.h", or;

  ```c
  #include "azure_c_shared_utility/shared_util_options.h" // for OPTION_OPENSSL_CIPHER_SUITE

  ...
  const char* ciphers = "list of ciphers, in the format expected by OpenSSL API's SSL_CTX_set_cipher_list()"; // please modify this string to have the desired ciphers.
  IoTHubDeviceClient_SetOption(iotHubClientHandle, OPTION_OPENSSL_CIPHER_SUITE, ciphers);
  ```

[IoTHubModuleClient_LL_SetOption](https://github.com/Azure/azure-iot-sdk-c/blob/2019-12-11/iothub_client/inc/iothub_module_client_ll.h#L252), if using "iothub_module_client_ll.h", or;

  ```c
  #include "azure_c_shared_utility/shared_util_options.h" // for OPTION_OPENSSL_CIPHER_SUITE

  ...
  const char* ciphers = "list of ciphers, in the format expected by OpenSSL API's SSL_CTX_set_cipher_list()"; // please modify this string to have the desired ciphers.
  IoTHubModuleClient_LL_SetOption(iotHubModuleClientHandle, OPTION_OPENSSL_CIPHER_SUITE, ciphers);
  ```

[IoTHubModuleClient_SetOption](https://github.com/Azure/azure-iot-sdk-c/blob/2019-12-11/iothub_client/inc/iothub_module_client.h#L233), if using "iothub_module_client.h", or;

  ```c
  #include "azure_c_shared_utility/shared_util_options.h" // for OPTION_OPENSSL_CIPHER_SUITE

  ...
  const char* ciphers = "list of ciphers, in the format expected by OpenSSL API's SSL_CTX_set_cipher_list()"; // please modify this string to have the desired ciphers.
  IoTHubModuleClient_SetOption(iotHubClientHandle, OPTION_OPENSSL_CIPHER_SUITE, ciphers);
  ```

### WolfSSL

Please check the [WolfSSL documentation](https://www.wolfssl.com/docs/wolfssl-manual/ch2/) to build the WolfSSL library to support the desired ciphers.

In its adapter layer, Azure IoT C SDK does not directly set the ciphers that wolfSSL should use.


### Apple iOS

Please follow the same guidelines above for using OpenSSL.


### mbedTLS

This [mbedTLS documentation](https://os.mbed.com/teams/sandbox/code/mbedtls/docs/bef26f687287/ssl_8h_source.html#l00477) provides details of how to set supported ciphers.

Any additional calls to mbedTLS API should be done in the Azure IoT C SDK mbedTLS adapter code ([tlsio_mbedtls.c, on tlsio_mbedtls.c after mbedtls_ssl_config_init](https://github.com/Azure/azure-c-shared-utility/blob/48f7a556865731f0e96c47eb5e9537361f24647c/adapters/tlsio_mbedtls.c#L482)). 


### BearSSL/esp8266 Arduino

Set the cipher suite while defining profile at initialization (see "Profiles" in the [BearSSL documentation](https://www.bearssl.org/api1.html)) or use [setCiphers](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/bearssl-client-secure-class.html). 

Any additional calls to BearSSL API should be done in the Azure IoT C SDK BearSSL adapter code ([tlsio_bearssl.c, on tlsio_bearssl_open after br_ssl_client_init_full](https://github.com/Azure/azure-c-shared-utility/blob/48f7a556865731f0e96c47eb5e9537361f24647c/adapters/tlsio_bearssl.c#L1102)).