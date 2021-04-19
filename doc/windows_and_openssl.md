# Using OpenSSL on IoT SDK on Windows

This document is only relevant if you are using the IoT C-SDK on Windows and you wish to use OpenSSL instead of Schannel for your TLS stack.

## Using OpenSSL on Windows is highly discouraged
By default you should use the SDK's default settings for Windows, which is to use Schannel.  You should avoid OpenSSL on Windows for the C-SDK because:

* OpenSSL/Windows combination for the SDK receives only basic testing and is not part of checkin gates.

* You are responsible for updating your OpenSSL dependencies as security fixes for it become available.  Schannel on Windows is a system component automatically serviced by Windows Update.  OpenSSL on Linux is updated by Linux packaging mechanisms, such as apt-get on Debian based distributions.  Shipping the C-SDK on Windows using OpenSSL means you are responsible for getting updated versions of it to your devices.

This document is kept primarily for archival purposes for customers who may be on OpenSSL/Windows and are not able to change.

## Setup instructions

[OpenSSL] binaries that the C-SDK depends on are **ssleay32** and **libeay32**. To enable OpenSSL to be used on Windows, you need to

- Obtain OpenSSL binaries.  There are many ways to do this, but one of the easier ways is to:

- Open the appropriate developer command prompt you plan on building the C-SDK from.

- Install [vcpkg], a Microsoft tool that helps you manage C and C++ libraries.

- Run `.\vcpkg install openssl` to obtain the required OpenSSL binaries.

- Make note of the directory that OpenSSL has been installed to by vcpkg, e.g. `C:\vcpkgRoot\vcpkg\packages\openssl_x86-windows`.

* Make the C-SDK link against these OpenSSL binaries instead of the default Schannel.
  * Regardless of how you obtained OpenSSL binaries, set environment variables to point at its root directory.  *Be careful there are no leading spaces between the `=` and directory name as cmake's errors are not always obvious.*
      ```Shell
       set OpenSSLDir=C:\vcpkgRoot\vcpkg\packages\openssl_x86-windows
       set OPENSSL_ROOT_DIR=C:\vcpkgRoot\vcpkg\packages\openssl_x86-windows
       ```
  * Build the SDK to include OpenSSL.  The key difference from the typical flow is the inclusion of the `-Duse_openssl:BOOL=ON` option when invoking cmake.

     ```Shell
     cd azure-iot-sdk-c
     mkdir cmake
     cd cmake
     cmake -Duse_openssl:BOOL=ON ..
     cmake --build . -- /m /p:Configuration=Release
     ```

## Samples and usage
Building samples and your application should be the same as using the default Schannel at this point.

[OpenSSL]:https://www.openssl.org/
