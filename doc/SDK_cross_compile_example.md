# Cross Compiling the Azure IoT Hub SDK
## Background

The SDK for the Azure IoT Hub was written to use the C99 standard in order to retain a high level of compatibility with hardware platforms that have not yet seen an official release of the SDK. This should ease the burden of porting the SDK code to those platforms.

One of the challenges that might be encountered though is that the platform in question is not capable of building the code. Many such platforms may require that the executables are compiled on a host with a different chipset architecture from its own. This process of building executables on a host to target a disparate platform is known as cross compiling.

## Purpose of this document

This document presents an example of how to cross compile the Azure IoT Hub SDK using the make system, [cmake](https://cmake.org), that is employed by the project. In this example it will demonstrate the process of cross compiling the SDK on a Debian 64-bit system targeting a Raspberry Pi. It demonstrates how one must set up the file system and the cmake options to achieve this which should assist developers attempting to cross compile for other targets.

## Procedure

### Version Information

The host machine is running Debian GNU/Linux 8 (jessie) amd64

The target machine is running Raspbian GNU/Linux 8 (jessie)

__Note:__ This example was built and tested on an __amd64__ build of Debian on the host and will use the __64-bit version__ of the Raspbian toolchain. You will need to select a different target toolchain if your host is not running a 64-bit Linux operating system.

Though it may be possible to use a host machine running a variant of Windows this would likely be very complex to set up and thus is not addressed in this document.

### Setting up the Build Environment

Open a terminal prompt on your host machine in the manner you prefer.

We need to acquire the SDK source code. This is available in the [C SDK GitHub repository](https://github.com/Azure/azure-iot-sdk-c.git). We clone this too our host machine as follows:

```Shell
cd ~
mkdir Source
cd Source
git clone https://github.com/Azure/azure-iot-sdk-c.git
cd azure-iot-sdk-c
git submodule update --init
```

Further information regarding this step and other set up requirements can be found in this [guide](https://github.com/Azure/azure-iot-sdk-c/blob/main/doc/devbox_setup.md). This step is only included in this document to establish the directory structure used for the rest of the example.

You might consider building the SDK for your local platform at this point simply to ensure you have all the required components. At the very least, you must ensure that the SDK's prerequisite libraries are installed on your Raspberry Pi. You can achieve this by running the script _setup.sh_ found in _azure-iot-sdk-c/build\_all/linux_.

In order to cross compile for a different target the first requirement is to set up an environment containing the required toolchain, system libraries and system headers that may be required to build the code. In the instance of the Raspberry Pi this is simplified by the existence of a GitHub project that has much of that work already done for us. Change to your home directory, create a new directory for the Raspberry Pi Tools and clone the project at https://github.com/raspberrypi/tools into that directory. For example:
```
cd ~
mkdir RPiTools
cd RPiTools
git clone https://github.com/raspberrypi/tools.git
```
Unfortunately, this does not give us all the files that are required to build the project. At this point you will need to copy some files from a running Raspberry Pi to your host machine. Make sure you can access your Raspberry Pi via SSH (you can enable this using [raspi-config](https://www.raspberrypi.org/documentation/configuration/raspi-config.md) and select "9. Advanced Options". Further instructions are beyond the scope of this document). 

Make sure [these libraries](https://github.com/Azure/azure-iot-sdk-c/blob/LTS_07_2020_Ref01/build_all/linux/setup.sh#L12) are installed in the Raspberry Pi before proceeding.

On your host's terminal enter:
```
cd ~/RPiTools/tools/arm-bcm2708/\
gcc-linaro-arm-linux-gnueabihf-raspbian-x64/arm-linux-gnueabihf
rsync -rl --exclude '/usr/lib/cups/backend/vnc' --safe-links pi@<your Pi identifier>:/{lib,usr} .
```
In the above command replace &lt;*your Pi identifier*&gt; with the IP address of your Raspberry Pi. If you no longer have a user identity on your Raspberry Pi called pi, then you will need to substitute an existing user identity.

This command will copy many more files to your host than you actually need but, for brevity, we copy them all. If you wish, you could optimize the sync action to exclude files that are not required.

### Building the SDK

In order to tell cmake that it is performing a cross compile we need to provide it with a toolchain file. To save us some typing and to make our whole procedure less dependent upon the current user we are going to export a value. Whilst in the same directory as above enter the following command
```
export RPI_ROOT=$(pwd)
```
You can use *export -p* to verify RPI\_ROOT has been added to the environment.
Then set the following variables accordingly:
```
ENV TOOLCHAIN_ROOT=${RPI_ROOT}/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf
ENV TOOLCHAIN_SYSROOT=${TOOLCHAIN_ROOT}
ENV TOOLCHAIN_EXES=${TOOLCHAIN_SYSROOT}/bin
ENV TOOLCHAIN_NAME=arm-linux-gnueabihf
ENV TOOLCHAIN_PREFIX=${TOOLCHAIN_SYSROOT}/usr
```
Now to create the toolchain and build the SDK, run the script found at:
[raspberrypi_c_buster.sh](https://github.com/Azure/azure-iot-sdk-c/blob/main/jenkins/raspberrypi_c_buster.sh)

```
#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -x # Set trace on
set -o errexit # Exit if command failed
set -o nounset # Exit if variable not set
set -o pipefail # Exit if pipe failed

cd ~/Source/azure-iot-sdk-c/

# assume directory is root of downloaded repo
mkdir cmake
cd cmake
ls -al

# Create a cmake toolchain file on the fly
echo "SET(CMAKE_SYSTEM_NAME Linux)     # this one is important" > toolchain.cmake
echo "SET(CMAKE_SYSTEM_VERSION 1)      # this one not so much" >> toolchain.cmake

echo "SET(CMAKE_C_COMPILER ${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-gcc)" >> toolchain.cmake
echo "SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-g++)" >> toolchain.cmake
echo "SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_SYSROOT})" >> toolchain.cmake
echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> toolchain.cmake
echo "SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> toolchain.cmake
echo "SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> toolchain.cmake
ls -al

# Build the SDK. This will use the OpenSSL, cURL and uuid binaries that we built before
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -Duse_prov_client:BOOL=OFF -DCMAKE_INSTALL_PREFIX=${TOOLCHAIN_PREFIX} -Drun_e2e_tests:BOOL=ON -Drun_unittests=:BOOL=ON ..
make -j 2
ls -al

```
### Specifying Additional Build Flags

If you need to provide additional build flags for your cross compile to function you can add further _-cl_ parameters to the build script's command line. For example, adding _-cl -v_ will turn on the verbose output from the compile and link process or to pass options only to the linker for example to pass -v to the linker you can use _-cl Wl,-v_.

See this page for a summary of available gcc flags: https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html.

### Notes
These instructions have been tested on both the Raspberry Pi 2, 3 and 4.

### Known Issues and Circumventions

If you encounter the error _error adding symbols: DSO missing from command line_ try adding a reference to libdl with  _-cl -ldl_ added to your build script command line.

For Raspberry Pi 4 the following workaround is necessary to properly link the samples with pthread.
Run the following commands and [recompile the Azure IoT C SDK](#Building-the-SDK).

```bash
cd $RPI_ROOT/usr/lib/arm-linux-gnueabihf; 
ln -s ../../../lib/arm-linux-gnueabihf/libpthread.so.0 libpthread.so
```

## Summary

This document has demonstrated how to cross compile the Azure IoT SDK on a 64-bit Debian host targeting the Raspbian operating system.

## References

<https://github.com/Azure/azure-iot-sdks>

<https://github.com/Azure/azure-iot-sdks/blob/main/c/doc/devbox_setup.md>

<https://github.com/raspberrypi/tools>

<https://cmake.org/Wiki/CMake_Cross_Compiling>

https://www.raspberrypi.org

<http://stackoverflow.com/questions/19162072/installing-raspberry-pi-cross-compiler> (See answer)
