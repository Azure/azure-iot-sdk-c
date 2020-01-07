# How to configure the Azure IoT C SDK and currently supported TLS platforms to use TLS 1.2 only 


## SChannel (Microsoft Windows)

To use exclusively TLS 1.2 in Microsoft Windows using SChannel, disable the support for TLS 1.0 and 1.1.

Be aware that this action might impact other applications in the systems that might not support higher versions of TLS.

To disable support in SChannel for TLS 1.0, follow this [documentation](https://docs.microsoft.com/en-us/windows-server/identity/ad-fs/operations/manage-ssl-protocols-in-ad-fs#enable-and-disable-tls-10).

To disable support in SChannel for TLS 1.1, follow this [documentation](https://docs.microsoft.com/en-us/windows-server/identity/ad-fs/operations/manage-ssl-protocols-in-ad-fs#enable-and-disable-tls-11).


## OpenSSL

The latest Azure IoT C SDK uses [TLS 1.2 by default with OpenSSL](https://github.com/Azure/azure-c-shared-utility/blob/48f7a556865731f0e96c47eb5e9537361f24647c/adapters/tlsio_openssl.c#L1167).

The SDK creates the SSL context using the method related to the TLS version selected (1.2 by default).

According to the [OpenSSL documentation](https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_new.html) the method selected causes OpenSSL to use only the TLS version associated with that method.  


## WolfSSL

To use only TLS 1.2 or higher, WolfSSL MUST be built without using “--enable-oldtls”.

Please check the [WolfSSL documentation](https://github.com/wolfSSL/wolfssl/wiki/Building-wolfSSL#25-build-options-configure-options) as this option is enabled by default on the binaries distributed by them.

In its adapter layer, Azure IoT C SDK does not directly set the TLS version that wolfSSL should use.
The result is that the wolfSSL library will use its default TLS version. 


## Apple iOS

TLS 1.2+ should already be the default for the latest versions of Apple iOS. 


## mbedTLS

mbedTLS can be configured to use only higher versions of TLS. 
That is achieved by calling  [mbedtls_ssl_conf_min_version](https://os.mbed.com/teams/sandbox/code/mbedtls/docs/bef26f687287/ssl_8h.html).

In its adapter layer, Azure IoT C SDK already [sets the minimum TLS version that mbedTLS should use](https://github.com/Azure/azure-c-shared-utility/blob/48f7a556865731f0e96c47eb5e9537361f24647c/adapters/tlsio_mbedtls.c#L481) to TLS 1.2.


## BearSSL/esp8266 Arduino

TLS version can be set on BearSSL by using [br_ssl_engine_set_versions](https://www.bearssl.org/gitweb/?p=BearSSL;a=blob;f=inc/bearssl_ssl.h;h=fee7b3c496086b96b5435674c130af2e740b1b88;hb=5f045c759957fdff8c85716e6af99e10901fdac0#l1114).

Currently the Azure IoT C SDK does not directly set the minimum version of TLS to be used by BearSSL.

