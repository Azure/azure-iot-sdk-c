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
> Be sure to include Visual C++ and NuGet Package Manager.

- Install [git]. Confirm git is in your PATH by typing `git version` from a command prompt.

- Install [CMake]. Make sure it is in your PATH by typing `cmake -version` from a command prompt. CMake will be used to create Visual Studio projects to build libraries and samples.

- Locate the tag name for the [latest release][latest-release] of the SDK.
> Our release tag names are date values in `yyyy-mm-dd` format.

- Clone the latest release of SDK to your local machine using the tag name you found:

```Shell
git clone -b <yyyy-mm-dd> --recursive https://github.com/Azure/azure-iot-sdk-c.git
```

> The `--recursive` argument instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

### Build a sample application without building the SDK

To quickly build one of our sample applications, open the corresponding [solution file][sln-file] (.sln) in Visual Studio.
  For example, to build the **telemetry message sample**, open **iothub_client\samples\iothub_ll_telemetry_sample\windows\iothub_ll_telemetry_sample.sln**.

In the sample's main source file, find the line similar to this:

```Shell
static const char* connectionString = "[device connection string]";
```

...and replace `[device connection string]` with a valid device connection string for a device registered with your IoT Hub. For more information, see the [samples section](#samplecode) below.

Build the sample project. As part of the build, [NuGet] Package Manager will download packages for dependencies so you don't have to build the entire SDK. See **windows\packages.config** for the list of packages that will be downloaded.

### Build the C SDK in Windows

In some cases, you may want to build the SDK locally for development and testing purposes. First, take the following steps to generate project files:

- Open a "Developer Command Prompt for VS2015" or "Developer Command Prompt for VS2017".

- Run the following CMake commands from the root of the repository:

```Shell
cd azure-iot-sdk-c
mkdir cmake
cd cmake
# Either
  cmake .. -G "Visual Studio 14 2015" .. ## For Visual Studio 2015
# or
  cmake .. -G "Visual Studio 15 2017" .. ## For Visual Studio 2017
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

For TLS operations, by default the C-SDK uses Schannel on Windows Platforms.  To enable OpenSSL to be used on Windows, you will need to execute the following instructions:

[OpenSSL] binaries that the C-SDK depends on are **ssleay32** and **libeay32**. You need to build and install these libraries and DLLs before you build the sample that uses them.

Below are steps to build and install OpenSSL libraries and corresponding DLLs. These steps were tested with **openssl-1.0.2k** on **Visual Studio 2015**.

- Go to the [OpenSSL Github Repository] and clone an appropriate release.

- For more details on supported configurations, prerequisites, and build steps read [OpenSSL Installation] and [Compilation and Installation].

- For x86 configuration, open "VS2015 x86 Native Tools Command Prompt" and follow the commands in the `INSTALL.W32` file.

- For x64 configuration, open "VS2015 x64 Native Tools Command Prompt" and following commands in the `INSTALL.W64` file.

After completing the above steps make sure OpenSSL libraries and DLLs are in your OpenSSL install location.

Follow these steps to build the sample:

- Open a Developer Command Prompt and change to the **build_all\\windows** directory

- Set OpenSSLDir and OPENSSL_ROOT_DIR **environment variables** to the OpenSSL install location. For example, if your OpenSSL install location is **C:\\usr\\local\\ssl**, you will set following:

   ```Shell
   set OpenSSLDir=C:\usr\local\ssl
   set OPENSSL_ROOT_DIR=C:\usr\local\ssl
   ```

- Build the SDK to include OpenSSL:

   ```Shell
   cd azure-iot-sdk-c
   mkdir cmake
   cd cmake
   cmake -Duse_openssl:BOOL=ON ..
   cmake --build . -- /m /p:Configuration=Release
   ```

This will build the C SDK libraries and use openssl as your TLS library.

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

- Locate the tag name for the [latest release][latest-release] of the SDK.
  > Our release tag names are date values in `yyyy-mm-dd` format.

- Clone the latest release of SDK to your local machine using the tag name you found:

  ```Shell
  git clone -b <yyyy-mm-dd> --recursive https://github.com/Azure/azure-iot-sdk-c.git
  ```

  > The `--recursive` argument instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

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
  git clone -b <yyyy-mm-dd> --recursive https://github.com/Azure/azure-iot-sdk-c.git
  ```
  > The `--recursive` argument instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

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

<a name="windowsce"></a>

## [**DEPRECATED**: Set up a Windows Embedded Compact 2013 development environment
***WINDOWS EMBEDDED COMPACT 2013 RECEIVES MINIMAL SUPPORT FROM AZURE IOT SDK.***  In particular please be aware:
* No new features will be added to WEC.  
* The only supported protocol is HTTPS.  Popular protocols like AMQP and MQTT, both over TCP directly and over WebSockets, are not and will not be supported.
* HTTPS over proxy is not supported.
* Developers may want to evaluate [Windows 10 IoT Core], which has much broader Azure IoT SDK support.

**SETUP INSTRUCTIONS FOR WINDOWS EMBEDDED COMPACT 2013**

- Install [Visual Studio 2015][visual-studio]. You can use the **Visual Studio Community** Free download if you meet the licensing requirements.
  > Be sure to include Visual C++ and NuGet Package Manager.

- Install [Application Builder for Windows Embedded Compact 2013][application-builder] for Visual Studio 2015
- Install [Toradex Windows Embedded Compact 2013 SDK][toradex-CE8-sdk] or your own SDK.

- Install [git]. Confirm git is in your PATH by typing `git version` from a command prompt.

- Install [CMake]. Make sure it is in your PATH by typing `cmake -version` from a command prompt. CMake will be used to create Visual Studio projects to build libraries and samples.

- Locate the tag name for the [latest release][latest-release] of the SDK.
  > Our release tag names are date values in `yyyy-mm-dd` format.

- Clone the latest release of SDK to your local machine using the tag name you found:

  ```Shell
  git clone -b <yyyy-mm-dd> --recursive https://github.com/Azure/azure-iot-sdk-c.git
  ```

  > The `--recursive` argument instructs git to clone other GitHub repos this SDK depends on. Dependencies are listed [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/.gitmodules).

If you installed a different SDK please check azure-iot-sdk-c\\build_all\\windowsce\\build.cmd and replace:

```Shell
set SDKNAME=TORADEX_CE800
set PROCESSOR=arm
```

with a reference to the name of the SDK and the processor architecture (arm/x86) you plan to use.

### Verify your environment

You can build the Windows samples to verify that your environment is set up correctly.

- Open a "Developer Command Prompt for VS2015".
- Navigate to the **build_all\\windowsce** folder in your local copy of the repository.
- Run the following command:

```Shell
build
```

This script uses CMake to create a folder called "cmake\_ce8" in your home directory and generates in that folder a Visual Studio solution called azure_iot_sdks.sln. The script will then proceed to build the **HTTP** sample.

> Note: you will not be able to run the samples until you configure them with a valid IoT hub device connection string. For more information, see [running a C sample application on Windows Embedded Compact 2013 on a Toradex module](https://github.com/Azure/azure-iot-device-ecosystem/blob/master/get_started/wince2013-toradex-module-c.md).

To view the projects and examine the source code, open the **azure_iot_sdks.sln** solution file in Visual Studio.

You can use one of the sample applications as a template to get started when you are creating your own client applications.

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
[NuGet]:https://www.nuget.org/
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
