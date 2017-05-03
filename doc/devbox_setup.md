# Prepare your development environment

This document describes how to prepare your development environment to use the **Microsoft Azure IoT device SDK for C**. It describes preparing a development environment in Windows using Visual Studio and in Linux.

- [Set up a Windows development environment](#windows)
- [Set up a Linux development environment](#linux)
- [Set up a macOS (Mac OS X) development environment](#macos)
- [Set up a Windows Embedded Compact 2013 development environment](#windowsce)
- [Sample applications](#samplecode)

<a name="windows"></a>
## Set up a Windows development environment

- Install [Visual Studio 2015][visual-studio]. You can use the **Visual Studio Community** Free download if you meet the licensing requirements.
  > Be sure to include Visual C++ and NuGet Package Manager.

- Install [git]. Confirm git is in your PATH by typing `git version` from a command prompt.

- Install [CMake]. Make sure it is in your PATH by typing `cmake -version` from a command prompt. CMake will be used to create Visual Studio projects to build libraries and samples.

- Locate the tag name for the [latest release][latest-release] of the SDK.
  > Our release tag names are date values in `yyyy-mm-dd` format.

- Clone the latest release of SDK to your local machine using the tag name you found:
  ```
  git clone -b <yyyy-mm-dd> --recursive https://github.com/Azure/azure-iot-sdk-c.git
  ```

  > The `--recursive` argument instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

### Build a sample application without building the SDK
To quickly build one of our sample applications, open the corresponding [solution file][sln-file] (.sln) in Visual Studio.
For example, to build our **MQTT sample**, open **iothub_client\samples\iothub_client_sample_mqtt\windows\iothub_client_sample_mqtt.sln**.

In the sample's main source file, find the line similar to this:
```
static const char* connectionString = "[device connection string]";
```
...and replace `[device connection string]` with a valid device connection string for a device registered with your IoT Hub. For more information, see the [samples section](#samplecode) below.

Build the sample project. As part of the build, [NuGet] Package Manager will download packages for dependencies so you don't have to build the entire SDK. See **windows\packages.config** for the list of packages that will be downloaded.

### Build the C SDK
In some cases, you may want to build the SDK locally for development and testing purposes. First, take the following steps to generate project files:
- Open a "Developer Command Prompt for VS2015".
- Run the following CMake commands from the root of the repository:

  ```
  cd azure-iot-sdk-c
  mkdir cmake
  cd cmake
  cmake -G "Visual Studio 14 2015" ..
  ```
  > This builds x86 libraries. To build for x64, modify the cmake generator argument: `cmake .. -G "Visual Studio 14 2015 Win64"`

If project generation completes successfully, you should see a Visual Studio solution file (.sln) under the `cmake` folder. To build the SDK, do one of the following:
- Open **cmake\azure_iot_sdks.sln** in Visual Studio and build it, **OR**
- Run the following command in the command prompt you used to generate the project files:
  ```
  cmake --build . -- /m /p:Configuration=Release
  ```
  > To build Debug binaries, use the corresponding MSBuild argument: `cmake --build . -- /m /p:Configuration=Debug`

  > There are many CMake configuration options available for building the SDK. For example, you can disable one of the available protocol stacks by adding an argument to the CMake project generation command:
  > ```
  > cmake -G "Visual Studio 14 2015" -Duse_amqp=OFF ..
  > ```
  > Also, you can build and run unit tests:
  > ```
  > cmake -G "Visual Studio 14 2015" -Drun_unittests=ON ..
  > cmake --build . -- /m /p:Configuration=Debug
  > ctest -C "debug" -V
  > ```

### Build a sample that uses WebSocket
**iothub_client_sample_amqp_websockets** (AMQP over WebSocket) depends on [OpenSSL] libraries **ssleay32** and **libeay32**. You need to build and install these libraries and DLLs before you build the sample that uses them.

Below are steps to build and install OpenSSL libraries and corresponding DLLs. These steps were tested with **openssl-1.0.2k**.

1. Go to the [OpenSSL Github Repository] and clone the release [git clone https://github.com/openssl/openssl.git -b OpenSSL_1_0_2k]

2. For more details on supported configurations, prerequisites, and build steps read [OpenSSL Installation] and [Compilation and Installation].

3. For x86 configuration, open "VS2015 x86 Native Tools Command Prompt" and follow the commands in the `INSTALL.W32` file.

4. For x64 configuration, open "VS2015 x64 Native Tools Command Prompt" and following commands in the `INSTALL.W64` file.

After completing the above steps make sure OpenSSL libraries and DLLs are in your OpenSSL install location.

Follow these steps to build the sample:

1. Open "Developer Command Prompt for VS2015" and change to the **build_all\\windows** directory

2. Set OpenSSLDir and OPENSSL_ROOT_DIR **environment variables** to the OpenSSL install location. For example, if your OpenSSL install location is **C:\\usr\\local\\ssl**, you will set following:
   ```
   set OpenSSLDir=C:\usr\local\ssl
   set OPENSSL_ROOT_DIR=C:\usr\local\ssl
   ```

3. Build the SDK, including the WebSocket sample:
   ```
   cd azure-iot-sdk-c
   mkdir cmake
   cd cmake
   cmake -G "Visual Studio 14 2015" ..
   cmake --build . -- /m /p:Configuration=Release
   ```

This will build the C SDK libraries along with **iothub_client_sample_amqp_websockets** sample.

<a name="linux"></a>
## Set up a Linux development environment

This section describes how to set up a development environment for the C SDK on [Ubuntu]. [CMake] will create makefiles and [make] will use them to compile the C SDK source code using the [gcc] compiler.

- Make sure all dependencies are installed before building the SDK. For Ubuntu, you can use apt-get to install the right packages:
  ```
  sudo apt-get update
  sudo apt-get install -y git cmake build-essential curl libcurl4-openssl-dev libssl-dev uuid-dev
  ```

- Verify that CMake is at least version **2.8.12**:
  ```
  cmake --version
  ```
  > For information about how to upgrade your version of CMake to 3.x on Ubuntu 14.04, read [How to install CMake 3.2 on Ubuntu 14.04?](http://askubuntu.com/questions/610291/how-to-install-cmake-3-2-on-ubuntu-14-04).

- Verify that gcc is at least version **4.4.7**:
  ```
  gcc --version
  ```
  > For information about how to upgrade your version of gcc on Ubuntu 14.04, read [How do I use the latest GCC 4.9 on Ubuntu 14.04?](http://askubuntu.com/questions/466651/how-do-i-use-the-latest-gcc-4-9-on-ubuntu-14-04).

- Locate the tag name for the [latest release][latest-release] of the SDK.
  > Our release tag names are date values in `yyyy-mm-dd` format.

- Clone the latest release of SDK to your local machine using the tag name you found:
  ```
  git clone -b <yyyy-mm-dd> --recursive https://github.com/Azure/azure-iot-sdk-c.git
  ```
  > The `--recursive` argument instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

### Build the C SDK
To build the SDK:

  ```
  cd azure-iot-sdk-c
  mkdir cmake
  cd cmake
  cmake ..
  cmake --build .  # append '-- -j <n>' to run <n> jobs in parallel
  ```
  > To build Debug binaries, add the corresponding CMake option to the project generation command above, e.g.:
  > ```
  > cmake -DCMAKE_BUILD_TYPE=Debug ..
  > ```

  > There are many CMake configuration options available for building the SDK. For example, you can disable one of the available protocol stacks by adding an argument to the CMake project generation command:
  > ```
  > cmake -Duse_amqp=OFF ..
  > ```
  > Also, you can build and run unit tests:
  > ```
  > cmake -Drun_unittests=ON ..
  > cmake --build .
  > ctest -C "debug" -V
  > ```

> Note: Any samples you built will not work until you configure them with a valid IoT Hub device connection string. For more information, see the [samples section](#samplecode) below.

<a name="macos"></a>
## Set up a macOS (Mac OS X) development environment

This section describes how to set up a development environment for the C SDK on [macOS]. [CMake] will create makefiles and [make] will use them to compile the C SDK source code using [clang]. [clang] is included with [XCode].

We've tested the device SDK for C on macOS Sierra, with XCode version 8.

- Make sure all dependencies are installed before building the SDK. For macOS, you can use [Homebrew] to install the right packages:
  ```
  brew update
  brew install git cmake pkgconfig openssl ossp-uuid
  ```

- Verify that [CMake] is at least version **2.8.12**:
  ```
  cmake --version
  ```

- Locate the tag name for the [latest release][latest-release] of the SDK.
  > Our release tag names are date values in `yyyy-mm-dd` format.

- Clone the latest release of SDK to your local machine using the tag name you found:
  ```
  git clone -b <yyyy-mm-dd> --recursive https://github.com/Azure/azure-iot-sdk-c.git
  ```
  > The `--recursive` argument instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

### Build the C SDK
To build the SDK:

  ```
  cd azure-iot-sdk-c
  mkdir cmake
  cd cmake
  cmake -DOPENSSL_ROOT_DIR:PATH=/usr/local/opt/openssl ..
  cmake --build .  # append '-- -j <n>' to run <n> jobs in parallel
  ```
  > To build Debug binaries, add the corresponding CMake option to the project generation command above, e.g.:
  > ```
  > cmake -DOPENSSL_ROOT_DIR:PATH=/usr/local/opt/openssl -DCMAKE_BUILD_TYPE=Debug ..
  > ```

  > There are many CMake configuration options available for building the SDK. For example, you can disable one of the available protocol stacks by adding an argument to the CMake project generation command:
  > ```
  > cmake -Duse_amqp=OFF ..
  > ```
  > Also, you can build and run unit tests:
  > ```
  > cmake -Drun_unittests=ON ..
  > cmake --build .
  > ctest -C "debug" -V
  > ```

> Note: Any samples you built will not work until you configure them with a valid IoT Hub device connection string. For more information, see the [samples section](#samplecode) below.

<a name="windowsce"></a>
## Set up a Windows Embedded Compact 2013 development environment

- Install [Visual Studio 2015][visual-studio]. You can use the **Visual Studio Community** Free download if you meet the licensing requirements.
  > Be sure to include Visual C++ and NuGet Package Manager.

- Install [Application Builder for Windows Embedded Compact 2013][application-builder] for Visual Studio 2015
- Install [Toradex Windows Embedded Compact 2013 SDK][toradex-CE8-sdk] or your own SDK.

- Install [git]. Confirm git is in your PATH by typing `git version` from a command prompt.

- Install [CMake]. Make sure it is in your PATH by typing `cmake -version` from a command prompt. CMake will be used to create Visual Studio projects to build libraries and samples.

- Locate the tag name for the [latest release][latest-release] of the SDK.
  > Our release tag names are date values in `yyyy-mm-dd` format.

- Clone the latest release of SDK to your local machine using the tag name you found:
  ```
  git clone -b <yyyy-mm-dd> --recursive https://github.com/Azure/azure-iot-sdk-c.git
  ```

  > The `--recursive` argument instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

If you installed a different SDK please check azure-iot-sdk-c\\build_all\\windowsce\\build.cmd and replace:
```
set SDKNAME=TORADEX_CE800
set PROCESSOR=arm
```
with a reference to the name of the SDK and the processor architecture (arm/x86) you plan to use.

### Verify your environment

You can build the Windows samples to verify that your environment is set up correctly.

1. Open a "Developer Command Prompt for VS2015".

2. Navigate to the **build_all\\windowsce** folder in your local copy of the repository.

3. Run the following command:

   ```
   build 
   ```

This script uses CMake to create a folder called "cmake\_ce8" in your home directory and generates in that folder a Visual Studio solution called azure_iot_sdks.sln. The script will then proceed to build the **HTTP** sample.

> Note: you will not be able to run the samples until you configure them with a valid IoT hub device connection string. For more information, see [running a C sample application on Windows Embedded Compact 2013 on a Toradex module](https://github.com/Azure/azure-iot-device-ecosystem/blob/master/get_started/wince2013-toradex-module-c.md).

To view the projects and examine the source code, open the **azure_iot_sdks.sln** solution file in Visual Studio.

You can use one of the sample applications as a template to get started when you are creating your own client applications.

<a name="samplecode"></a>
## Sample applications

This repository contains various C sample applications that illustrate how to use the Azure IoT device SDK for C:
* [Simple samples for the device SDK][simple_samples]
* [Samples using the serializer library][serializer_samples]


[visual-studio]: https://www.visualstudio.com/downloads/
[device-explorer]: https://github.com/Azure/azure-iot-sdks/tree/master/tools/DeviceExplorer
[toradex-CE8-sdk]:http://docs.toradex.com/102578
[application-builder]:http://www.microsoft.com/download/details.aspx?id=38819
[azure-shared-c-utility]:https://github.com/Azure/azure-c-shared-utility
[azure-uamqp-c]:https://github.com/Azure/azure-uamqp-c
[azure-umqtt-c]:https://github.com/Azure/azure-umqtt-c
[sln-file]:https://msdn.microsoft.com/en-us/library/bb165951.aspx
[git]:http://www.git-scm.com
[NuGet]:https://www.nuget.org/
[CMake]:https://cmake.org/
[MSBuild]:https://msdn.microsoft.com/en-us/library/0k6kkbsd.aspx
[OpenSSL]:https://www.openssl.org/
[OpenSSL Repository]: https://github.com/openssl/openssl
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
