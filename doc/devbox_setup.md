# Prepare your development environment

This document describes how to prepare your development environment to use the **Microsoft Azure IoT device SDK for C**.

- [Set up a Windows development environment](#windows)
- [Set up a Linux development environment](#linux)
- [Set up a macOS (Mac OS X) development environment](#macos)
- [Sample applications](#samplecode)

<a name="windows"></a>

## Set up a Windows development environment

- Install [Visual Studio 2019][visual-studio]. You can use the **Visual Studio Community** free download if you meet the licensing requirements.  (**Visual Studio 2017** is also supported.)

> Be sure to include Visual C++.

- Install [git]. Confirm git is in your PATH by typing `git version` from a command prompt.

- Install CMake, either by including it in your Visual Studio 2019 install or installing directly from [CMake.org][CMake]. Make sure it is in your PATH by typing `cmake -version` from a command prompt. CMake will be used to create Visual Studio projects to build libraries and samples.

- Locate the tag name for the [latest release][latest-release] of the SDK.

> Our release tag names are date values in `lts_mm_yyyy` format.

- Clone the latest release of SDK to your local machine using the tag name you found:

```Shell
git clone -b <lts_mm_yyyy> https://github.com/Azure/azure-iot-sdk-c.git
cd azure-iot-sdk-c
git submodule update --init
```

>If you are using a release before 2019-04-15 then you will need to use the `--recursive` argument for `git submodule`.  Otherwise it is highly recommended NOT to use `--recursive` as this results in poor git performance.

### Building sample applications using vcpkg to build the SDK

The sample applications can be build with the help of the C SDK libraries and headers built with vcpkg.  vcpkg is a package manager that facilitates building C and C++ libraries. To install the vcpkg C SDK libraries and headers, follow these steps [Setup C SDK vcpkg for Windows development environment](setting_up_vcpkg.md#setup-c-sdk-vcpkg-for-windows-development-environment).

Note: vcpkg manager creates a directory with all the headers and generates the C SDK .lib files on your machine. If you are using Visual Studio (from 2017) the command 'vcpkg integrate install' lets Visual Studio knows where the vcpkg headers and lib directories are located. If you're using other IDEs, just add the vcpkg directories to your compiler/linker include paths.

To quickly build one of the sample applications, open the corresponding Visual Studio [solution file][sln-file] (.sln) in Visual Studio.
  For example, to build the **telemetry message sample**, open **iothub_client\samples\iothub_ll_telemetry_sample\windows\iothub_ll_telemetry_sample.sln**.

In the sample's main source file, find the line similar to this:

```C
static const char* connectionString = "[device connection string]";
```

...and replace `[device connection string]` with a valid device connection string for a device registered with your IoT Hub. For more information, see the [samples section](#samplecode) below.

Build the sample project.


### Building the C SDK with CMake on Windows

In some cases, you may want to build the SDK locally for development and testing purposes (without using vcpkg). First, take the following steps to generate project files:

- Open a "Developer Command Prompt for VS2017" or "Developer Command Prompt for VS2019".

- Run the following CMake commands from the root of the repository:

```Shell
cd azure-iot-sdk-c
mkdir cmake
cd cmake
# Either
  cmake .. -G "Visual Studio 15 2017" ## For Visual Studio 2017
# or
  cmake .. -G "Visual Studio 16 2019" -A Win32
```

> This builds x86 libraries. To build for x64 for Visual Studio 2017, `cmake .. -G "Visual Studio 15 2017 Win64"` or for Visual Studio 2019, `cmake .. -G "Visual Studio 16 2019 -A x64"`

When the project generation completes successfully, you will see a Visual Studio solution file (.sln) under the `cmake` folder. To build the SDK, do one of the following:

- Open **cmake\azure_iot_sdks.sln** in Visual Studio and build it, **OR**
- Run the following command in the command prompt you used to generate the project files:

```Shell
cmake --build . -- /m /p:Configuration=Release
```

> To build Debug binaries, use the corresponding MSBuild argument: `cmake --build . -- /m /p:Configuration=Debug`
> There are many CMake configuration options available for building the SDK. For example, you can disable one of the available protocol stacks by adding an argument to the CMake project generation command:

```Shell
cmake -G "Visual Studio 15 2017" -Duse_amqp=OFF .. // same with 2017 and 2019 generator (see above)
```

> Also, you can build and run unit tests:

```Shell
cmake -G "Visual Studio 15 2017" -Drun_unittests=ON ..  // same with 2017 and 2019 generator (see above)
cmake --build . -- /m /p:Configuration=Debug
ctest -C "debug" -V
```

### Build a sample that uses different protocols, including over WebSockets

By default the C-SDK will have web sockets enabled for AMQP and MQTT.  The sample **iothub_client\samples\iothub_ll_telemetry_sample** lets you specify the `protocol` variable.  

Unless you have a reason otherwise, we recommend you use MQTT as your protocol.

### Build a sample using a specific server root certificate

By default all samples are built to handle a variety of server root CA certificates during TLS negotiation. If you would like to decrease the size of the sample, select the appropriate root certificate option during the cmake step. Follow the instructions [here](https://github.com/Azure/azure-iot-sdk-c/certs).

> Note: Any samples you build will not work until you configure them with a valid IoT Hub device connection string. For more information, see the [samples section](#samplecode) below.

It is possible to use OpenSSL as your TLS stack instead of the default Schannel when running on Windows, although this is *highly* discouraged.  See [this document][windows-and-openssl] for more.

<a name="linux"></a>

## Set up a Linux development environment

This section describes how to set up a development environment for the C SDK on [Ubuntu]. [CMake] will create makefiles and [make] will use them to compile the C SDK source code using the [gcc] compiler.

- Make sure all dependencies are installed before building the SDK. For Ubuntu, you can use apt-get to install the right packages:

  ```Shell
  sudo apt-get update
  sudo apt-get install -y git cmake build-essential curl libcurl4-openssl-dev libssl-dev uuid-dev ca-certificates
  ```
  NOTE: If you are planning to use HTTP with wolfSSL, you must configure curl before installation.
  1. Download the latest [curl](https://github.com/curl/curl/releases).
  2. See curl [documentation](https://curl.se/docs/install.html) to use the wolfSSL option for configuration.  Install.

- Verify that CMake is at least version **2.8.12**:

  ```Shell
  cmake --version
  ```

- Verify that gcc is at least version **4.4.7**:

  ```Shell
  gcc --version
  ```

- Locate the tag name for the [latest release][latest-release] of the SDK.
  > Our release tag names are date values in `lts_mm_yyyy` format.

- Clone the latest release of SDK to your local machine using the tag name you found:

  ```Shell
  git clone -b <lts_mm_yyyy> https://github.com/Azure/azure-iot-sdk-c.git
  cd azure-iot-sdk-c
  git submodule update --init
  ```

  >If you are using a release before 2019-04-15 then you will need to use the `--recursive` argument for `git submodule`.  Otherwise it is highly recommended NOT to use `--recursive` as this results in poor git performance.

### Build the C SDK on Linux

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

### Build a sample that uses different protocols, including over WebSockets

By default the C-SDK will have web sockets enabled for AMQP and MQTT.  The sample **iothub_client\samples\iothub_ll_telemetry_sample** lets you specify the `protocol` variable.

Unless you have a reason otherwise, we recommend you use MQTT as your protocol.

### Build a sample using a specific server root certificate

By default all samples are built to handle a variety of server root CA certificates during TLS negotiation. If you would like to decrease the size of the sample, select the appropriate root certificate option during the cmake step. Follow the instructions [here](https://github.com/Azure/azure-iot-sdk-c/certs).

> Note: Any samples you build will not work until you configure them with a valid IoT Hub device connection string. For more information, see the [samples section](#samplecode) below.

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
  > Our release tag names are date values in `lts_mm_yyyy` format.

- Clone the latest release of SDK to your local machine using the tag name you found:

  ```Shell
  git clone -b <lts_mm_yyyy> https://github.com/Azure/azure-iot-sdk-c.git
  cd azure-iot-sdk-c
  git submodule update --init
  ```

  >If you are using a release before 2019-04-15 then you will need to use the `--recursive` argument for `git submodule`.  Otherwise it is highly recommended NOT to use `--recursive` as this results in poor git performance.

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

### Build a sample that uses different protocols, including over WebSockets

By default the C-SDK will have web sockets enabled for AMQP and MQTT.  The sample **iothub_client\samples\iothub_ll_telemetry_sample** lets you specify the `protocol` variable.

Unless you have a reason otherwise, we recommend you use MQTT as your protocol.

### Build a sample using a specific server root certificate

By default all samples are built to handle a variety of server root CA certificates during TLS negotiation. If you would like to decrease the size of the sample, select the appropriate root certificate option during the cmake step. Follow the instructions [here](https://github.com/Azure/azure-iot-sdk-c/certs).

> Note: Any samples you build will not work until you configure them with a valid IoT Hub device connection string. For more information, see the [samples section](#samplecode) below.

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

-[Samples for the device SDK][device_samples]

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
[azure-iot-explorer]: https://github.com/Azure/azure-iot-explorer
[toradex-CE8-sdk]:http://docs.toradex.com/102578
[application-builder]:http://www.microsoft.com/download/details.aspx?id=38819
[azure-shared-c-utility]:https://github.com/Azure/azure-c-shared-utility
[azure-uamqp-c]:https://github.com/Azure/azure-uamqp-c
[azure-umqtt-c]:https://github.com/Azure/azure-umqtt-c
[sln-file]:https://msdn.microsoft.com/library/bb165951.aspx
[git]:http://www.git-scm.com
[CMake]:https://cmake.org/
[MSBuild]:https://msdn.microsoft.com/library/0k6kkbsd.aspx
[Compilation and Installation]:https://wiki.openssl.org/index.php/Compilation_and_Installation#Windows
[Ubuntu]:http://www.ubuntu.com/desktop
[gcc]:https://gcc.gnu.org/
[make]:https://www.gnu.org/software/make/
[macOS]:http://www.apple.com/macos/
[XCode]:https://developer.apple.com/xcode/
[Homebrew]:http://brew.sh/
[clang]:https://clang.llvm.org/
[device_samples]: ../iothub_client/samples/
[latest-release]:https://github.com/Azure/azure-iot-sdk-c/releases/latest
[Windows 10 IoT Core]:https://docs.microsoft.com/windows/iot-core/develop-your-app/buildingappsforiotcore
[vcpkg]:https://github.com/Microsoft/vcpkg
[windows-and-openssl]:./windows_and_openssl.md
