# Prepare your development environment

This document describes how to prepare your development environment to use the **Microsoft Azure IoT device SDK for C**. It describes preparing a development environment in Windows using Visual Studio and in Linux.

- [Set up a Windows development environment](#windows)
- [Set up a Linux development environment](#linux)
- [Set up a macOS (Mac OS X) development environment](#macos)
- [Set up a Windows Embedded Compact 2013 development environment](#windowsce)
- [Sample applications](#samplecode)

<a name="windows"></a>

## Set up a Windows development environment

- Install [Visual Studio 2017][visual-studio]. You can use the **Visual Studio Community** Free download if you meet the licensing requirements.  (**Visual Studio 2015** is also supported.)

> Be sure to include Visual C++.

- Install [git]. Confirm git is in your PATH by typing `git version` from a command prompt.

- Install [CMake]. Make sure it is in your PATH by typing `cmake -version` from a command prompt. CMake will be used to create Visual Studio projects to build libraries and samples.

- Locate the tag name for the [latest release][latest-release] of the SDK.

> Our release tag names are date values in `yyyy-mm-dd` format.

- Clone the latest release of SDK to your local machine using the tag name you found:

```Shell
git clone -b <yyyy-mm-dd> https://github.com/Azure/azure-iot-sdk-c.git
cd azure-iot-sdk-c
git submodule update --init
```

> If you are using a release before 2019-04-15 then you will need to use the `--recursive` argument to instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

### Build a sample application without building the SDK

The sample applications build with the help of C SDK libraries and headers published from vcpkg (a library manager). To install the C SDK libraries and headers, follow the steps to [Setup C SDK vcpkg for Windows development environment](https://github.com/Azure/azure-iot-sdk-c/blob/master/doc/setting_up_vcpkg.md#setup-c-sdk-vcpkg-for-windows-development-environment).

To quickly build one of our sample applications, open the corresponding [solution file][sln-file] (.sln) in Visual Studio.
  For example, to build the **telemetry message sample**, open **iothub_client\samples\iothub_ll_telemetry_sample\windows\iothub_ll_telemetry_sample.sln**.

In the sample's main source file, find the line similar to this:

```Shell
static const char* connectionString = "[device connection string]";
```

...and replace `[device connection string]` with a valid device connection string for a device registered with your IoT Hub. For more information, see the [samples section](#samplecode) below.

Build the sample project.

### Build the C SDK in Windows

In some cases, you may want to build the SDK locally for development and testing purposes. First, take the following steps to generate project files:

- Open a "Developer Command Prompt for VS2015" or "Developer Command Prompt for VS2017".

- Run the following CMake commands from the root of the repository:

```Shell
cd azure-iot-sdk-c
mkdir cmake
cd cmake
# Either
  cmake .. -G "Visual Studio 14 2015" ## For Visual Studio 2015
# or
  cmake .. -G "Visual Studio 15 2017" ## For Visual Studio 2017
```

> This builds x86 libraries. To build for x64 for Visual Studio 2015, modify the cmake generator argument: `cmake .. -G "Visual Studio 14 2015 Win64"` or for Visual Studio 2017, `cmake .. -G "Visual Studio 15 2017 Win64"`

If project generation completes successfully, you should see a Visual Studio solution file (.sln) under the `cmake` folder. To build the SDK, do one of the following:

- Open **cmake\azure_iot_sdks.sln** in Visual Studio and build it, **OR**
- Run the following command in the command prompt you used to generate the project files:

```Shell
cmake --build . -- /m /p:Configuration=Release
```

> To build Debug binaries, use the corresponding MSBuild argument: `cmake --build . -- /m /p:Configuration=Debug`
> There are many CMake configuration options available for building the SDK. For example, you can disable one of the available protocol stacks by adding an argument to the CMake project generation command:

```Shell
cmake -G "Visual Studio 14 2015" -Duse_amqp=OFF ..
```

> Also, you can build and run unit tests:

```Shell
cmake -G "Visual Studio 14 2015" -Drun_unittests=ON ..
cmake --build . -- /m /p:Configuration=Debug
ctest -C "debug" -V
```

### Build a sample that uses different protocols, including over WebSockets

By default the C-SDK will have web sockets enabled for AMQP and MQTT.  The sample **iothub_client\samples\iothub_ll_telemetry_sample** lets you specify the `protocol` variable.

### Using OpenSSL in the SDK

For TLS operations, by default the C-SDK uses Schannel on Windows Platforms.  You can use OpenSSL instead on Windows if you desire.  **NOTE:  this configuration presently receives only basic manual testing and does not have gated e2e tests.**

**IMPORTANT SECURITY NOTE WHEN USING OPENSSL ON WINDOWS**  
You are responsible for updating your OpenSSL dependencies as security fixes for it become available.  Schannel on Windows is a system component automatically serviced by Windows Update.  OpenSSL on Linux is updated by Linux packaging mechanisms, such as apt-get on Debian based distributions.  Shipping the C-SDK on Windows using OpenSSL means you are responsible for getting updated versions of it to your devices.

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

Building samples and your application should be the same as using the default Schannel at this point.

<a name="linux"></a>

## Set up a Linux development environment

This section describes how to set up a development environment for the C SDK on [Ubuntu]. [CMake] will create makefiles and [make] will use them to compile the C SDK source code using the [gcc] compiler.

- Make sure all dependencies are installed before building the SDK. For Ubuntu, you can use apt-get to install the right packages:

  ```Shell
  sudo apt-get update
  sudo apt-get install -y git cmake build-essential curl libcurl4-openssl-dev libssl-dev uuid-dev
  ```

- Verify that CMake is at least version **2.8.12**:

  ```Shell
  cmake --version
  ```

  > For information about how to upgrade your version of CMake to 3.x on Ubuntu 14.04, read [How to install CMake 3.2 on Ubuntu 14.04?](http://askubuntu.com/questions/610291/how-to-install-cmake-3-2-on-ubuntu-14-04).

- Verify that gcc is at least version **4.4.7**:

  ```Shell
  gcc --version
  ```

  > For information about how to upgrade your version of gcc on Ubuntu 14.04, read [How do I use the latest GCC 4.9 on Ubuntu 14.04?](http://askubuntu.com/questions/466651/how-do-i-use-the-latest-gcc-4-9-on-ubuntu-14-04).

- Patch CURL to the latest version available.
  > The minimum version of curl required is 7.56, as recent previous versions [presented critical failures](https://github.com/Azure/azure-iot-sdk-c/issues/308).
  > To upgrade see "Upgrade CURL on Mac OS" steps below.
  
- Locate the tag name for the [latest release][latest-release] of the SDK.
  > Our release tag names are date values in `yyyy-mm-dd` format.

- Clone the latest release of SDK to your local machine using the tag name you found:

  ```Shell
  git clone -b <yyyy-mm-dd> https://github.com/Azure/azure-iot-sdk-c.git
  cd azure-iot-sdk-c
  git submodule update --init
  ```

  > If you are using a release before 2019-04-15 then you will need to use the `--recursive` argument to instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

### Build the C SDK in Linux

To build the SDK:

```Shell
cd azure-iot-sdk-c
mkdir cmake
cd cmake
cmake ..
cmake --build .  # append '-- -j <n>' to run <n> jobs in parallel
```

> To build Debug binaries, add the corresponding CMake option to the project generation command above, e.g.:

```Shell
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

There are many CMake configuration options available for building the SDK. For example, you can disable one of the available protocol stacks by adding an argument to the CMake project generation command:

```Shell
cmake -Duse_amqp=OFF ..
```

Also, you can build and run unit tests:

```Shell
cmake -Drun_unittests=ON ..
cmake --build .
ctest -C "debug" -V
```

> Note: Any samples you built will not work until you configure them with a valid IoT Hub device connection string. For more information, see the [samples section](#samplecode) below.

<a name="macos"></a>

## Set up a macOS (Mac OS X) development environment

This section describes how to set up a development environment for the C SDK on [macOS]. [CMake] will 
create an XCode project containing the various SDK libraries and samples.

We've tested the device SDK for C on macOS High Sierra, with XCode version 9.2.

- Make sure all dependencies are installed before building the SDK. For macOS, you can use [Homebrew] to install the right packages:

  ```Shell
  brew update
  brew install git cmake pkgconfig openssl ossp-uuid
  ```

- Verify that [CMake] is at least version **2.8.12**:

  ```Shell
  cmake --version
  ```

- Patch CURL to the latest version available.
  > The minimum version of curl required is 7.56, as recent previous versions [presented critical failures](https://github.com/Azure/azure-iot-sdk-c/issues/308).
  > To upgrade see "Upgrade CURL on Mac OS" steps below.
  
- Locate the tag name for the [latest release][latest-release] of the SDK.
  > Our release tag names are date values in `yyyy-mm-dd` format.

- Clone the latest release of SDK to your local machine using the tag name you found:

  ```Shell
  git clone -b <yyyy-mm-dd> https://github.com/Azure/azure-iot-sdk-c.git
  cd azure-iot-sdk-c
  git submodule update --init
  ```

  > If you are using a release before 2019-04-15 then you will need to use the `--recursive` argument to instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

#### Upgrade CURL on Mac OS

1. Install the latest version of curl
```macosx:~ username$ brew install curl```

At the end of the installation you will get this info:
```
Error: Failed to create /usr/local/opt/curl
...
==> Caveats
This formula is keg-only, which means it was not symlinked into /usr/local,
because macOS already provides this software and installing another version in
parallel can cause all kinds of trouble.
...
==> Summary
🍺  /usr/local/Cellar/curl/7.58.0: 415 files, 3MB
```
Save the path "/usr/local/Cellar/curl/7.58.0", this is where the library and headers were actually installed.

2. Force link updates:

```
brew link curl --force
```

3. Set DYLD_LIBRARY_PATH:
```
export DYLD_LIBRARY_PATH="/usr/local/Cellar/curl/7.58.0/lib:$DYLD_LIBRARY_PATH"
```

Make sure the path above matches the one saved on step 1.
  
### Build the C SDK

If you upgraded CURL, make sure to set DYLD_LIBRARY_PATH each time you open a new shell.
The next command assumes curl was saved on the path below mentioned ("keg install"). 
Make sure you use the path as informed by `brew` when curl was upgraded.

```
export DYLD_LIBRARY_PATH="/usr/local/Cellar/curl/7.58.0/lib:$DYLD_LIBRARY_PATH"
``` 

Note: Any samples you built will not work until you configure them with a valid IoT Hub device connection string. For more information, see the [samples section](#samplecode) below.

Next you can either build the C SDK with CMake directly, or you can use CMake to generate an XCode project.

#### Building the C SDK with CMake directly

```Shell
cd azure-iot-sdk-c
mkdir cmake
cd cmake
cmake -DOPENSSL_ROOT_DIR:PATH=/usr/local/opt/openssl ..
cmake --build .  # append '-- -j <n>' to run <n> jobs in parallel
```

  > To build Debug binaries, add the corresponding CMake option to the project generation command above, e.g.:

```Shell
cmake -DOPENSSL_ROOT_DIR:PATH=/usr/local/opt/openssl -DCMAKE_BUILD_TYPE=Debug ..
```

There are many CMake configuration options available for building the SDK. For example, you can disable one of the available protocol stacks by adding an argument to the CMake project generation command:

  > ```Shell
  > cmake -Duse_amqp=OFF ..
  > ```

Also, you can build and run unit tests:

  > ```Shell
  > cmake -Drun_unittests=ON ..
  > cmake --build .
  > ctest -C "debug" -V
  > ```

#### Generate an XCode Project

To generate the XCode project:

```Shell
cd azure-iot-sdk-c
mkdir cmake
cd cmake
cmake -G Xcode ..
```
All of the CMake options described above will work for XCode generation as well.

When project generation completes you will see an XCode project file (.xcodeproj) under 
the `cmake` folder. To build the SDK, open **cmake\azure_iot_sdks.xcodeproj** in XCode and 
use XCode's build and run features.

**Note:** Until Mac updates the Curl library to version to 7.58 or greater it will also be necessary
to modify your XCode project file (.xcodeproj) to replace all the lines that read</br>
`LIBRARY_SEARCH_PATHS = "";`</br>
with</br>
`LIBRARY_SEARCH_PATHS = "/usr/local/Cellar/curl/7.58.0/lib";`</br>

The example above assumes curl 7.58 has been compiled and saved into `/usr/local/Cellar/curl/7.58.0`. For more details please see section "Upgrade CURL on Mac OS".

<a name="samplecode"></a>

## Sample applications

This repository contains various C sample applications that illustrate how to use the Azure IoT device SDK for C:

-[Simple samples for the device SDK][simple_samples]

-[Samples using the serializer library][serializer_samples]

Once the SDK is building successfully you can run any of the available samples using the following (example given for a linux system):

-Edit the sample file entering the proper connection string from the Azure Portal:

```c
static const char* connectionString = "[device connection string]";
```

-Navigate to the directory of the sample that was edited, build the sample with the new changes and execute the sample:

```Shell
cd ./azure-iot-sdk-c/cmake/iothub_client/samples/iothub_ll_telemetry_sample
make
./iothub_ll_telemetry_sample
```

[visual-studio]: https://www.visualstudio.com/downloads/
[device-explorer]: https://github.com/Azure/azure-iot-sdks/tree/master/tools/DeviceExplorer
[toradex-CE8-sdk]:http://docs.toradex.com/102578
[application-builder]:http://www.microsoft.com/download/details.aspx?id=38819
[azure-shared-c-utility]:https://github.com/Azure/azure-c-shared-utility
[azure-uamqp-c]:https://github.com/Azure/azure-uamqp-c
[azure-umqtt-c]:https://github.com/Azure/azure-umqtt-c
[sln-file]:https://msdn.microsoft.com/en-us/library/bb165951.aspx
[git]:http://www.git-scm.com
[CMake]:https://cmake.org/
[MSBuild]:https://msdn.microsoft.com/en-us/library/0k6kkbsd.aspx
[OpenSSL]:https://www.openssl.org/
[OpenSSL Github Repository]: https://github.com/openssl/openssl
[OpenSSL Installation]:https://github.com/openssl/openssl/blob/master/INSTALL
[Compilation and Installation]:https://wiki.openssl.org/index.php/Compilation_and_Installation#Windows
[Ubuntu]:http://www.ubuntu.com/desktop
[gcc]:https://gcc.gnu.org/
[make]:https://www.gnu.org/software/make/
[macOS]:http://www.apple.com/macos/
[XCode]:https://developer.apple.com/xcode/
[Homebrew]:http://brew.sh/
[clang]:https://clang.llvm.org/
[simple_samples]: ../iothub_client/samples/
[serializer_samples]: ../serializer/samples/
[latest-release]:https://github.com/Azure/azure-iot-sdk-c/releases/latest
[Windows 10 IoT Core]:https://docs.microsoft.com/en-us/windows/iot-core/develop-your-app/buildingappsforiotcore
[vcpkg]:https://github.com/Microsoft/vcpkg