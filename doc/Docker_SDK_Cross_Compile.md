# Cross Compiling the SDK in a Docker Container
## Background
The Microsoft Azure IoT SDK for C is written in C99 standard code. This allows the SDK to be compile and run on a wide variety of platforms. However many of those platforms are incapable of actually compiling the SDK. To address this deficiency one can compile the SDK on a different host, typically a Linux installation, using a cross compile toolchain that will create binaries for one's target platform.

Unfortunately this is not always an easy operation. The IoT SDK has dependencies on some other third party open source projects. These may or may not be available in binary form for your selected platform.
## Purpose of this Document
This document demonstrates how one can cross compile the SDK inside a Docker container. It consists of a Docker script that builds the SDK for device that is too small to build it on the device. In this instance I am targeting a [WRTNode](http://www.linksprite.com/wiki/index.php5?title=WRTnode) device which runs a version of OpenWrt, a commonly found Linux distribution for small devices. 
## Prerequisites
You will need to have a working installation of Docker in order to run the Docker script. However, should you choose not to build the SDK in a Docker container, you will still be able to run the command sequence on a Linux distribution based on Debian such as Ubuntu.
## Process
The Docker container is based on the latest Ubuntu Docker container. From there we will first install the required Ubuntu packages needed to run the rest of the commands. These are:
* git - required to clone the Azure IoT SDK for C
* cmake - used to process the CMakeLists.txt files to create makefiles
* wget - used to acquire the source of various other components
* nano - in case one needs to edit any files in the Docker container

Once this is complete, the script will acquire all of the prerequisites. These are:
* The toolchain that will build binaries for our target platform. You should substitute the toolchain for your target platform
* [The IoT SDK source code](https://github.com/azure/azure-iot-sdk-c).
* [OpenSSL](https://www.openssl.org/) - version 1.0.2o used
* [cURL](https://curl.haxx.se/) - version 7.60.0 used
* [util-linux](https://en.wikipedia.org/wiki/Util-linux) for uuid functionality.

With all of those in place we can build OpenSSL, cURL, and libuuid. These will be installed into the toolchain as each is built and will be used in the final step when the SDK itself is built.

## Major Tasks
You will need to identify a suitable toolchain for your target platform and modify the line that downloads this toolchain in your Docker script. 

Once this is done, then the environment variable set below will need to be modified to reflect the location and directory names of that toolchain. Once these have been updated then the Docker script should be ready to run.

## The Docker Script
Here is an example script:
```docker
# Start with the latest version of the Ubuntu Docker container
FROM ubuntu:latest

# Run commands that require root authority

# Fetch and install all outstanding updates
RUN apt-get update && apt-get -y upgrade

# Install cmake, git, wget and nano
RUN apt-get install -y cmake git wget nano

# Create a user to use so that we don't run it all as root
RUN useradd -d /home/builder -ms /bin/bash -G sudo -p builder builder

# Switch to new user and change to the user's home directory
USER builder
WORKDIR /home/builder

# Create a work directory and switch to it
RUN mkdir MIPSBuild
WORKDIR MIPSBuild

#
# The following wget invocation will need to be modified to download a toolchain appropriate 
# for your target device.
#

# Download the WRTNode cross compile toolchain and expand it
RUN wget https://downloads.openwrt.org/barrier_breaker/14.07/ramips/mt7620n/OpenWrt-Toolchain-ramips-for-mipsel_24kec%2bdsp-gcc-4.8-linaro_uClibc-0.9.33.2.tar.bz2
RUN tar -xvf OpenWrt-Toolchain-ramips-for-mipsel_24kec+dsp-gcc-4.8-linaro_uClibc-0.9.33.2.tar.bz2

# Download the Azure IoT SDK for C
RUN git clone https://github.com/azure/azure-iot-sdk-c.git
RUN cd azure-iot-sdk-c
RUN git submodule update --init

# Download OpenSSL source and expand it
RUN wget https://www.openssl.org/source/openssl-1.0.2o.tar.gz
RUN tar -xvf openssl-1.0.2o.tar.gz

# Download cURL source and expand it
RUN wget http://curl.haxx.se/download/curl-7.60.0.tar.gz
RUN tar -xvf curl-7.60.0.tar.gz

# Download the Linux utilities for libuuid and expand it
RUN wget https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.32/util-linux-2.32-rc2.tar.gz
RUN tar -xvf util-linux-2.32-rc2.tar.gz

#
# Set up environment variables in preparation for the builds to follow
# These will need to be modified for the corresponding locations in the downloaded toolchain
#
ENV WORK_ROOT=/home/builder/MIPSBuild
ENV TOOLCHAIN_ROOT=${WORK_ROOT}/OpenWrt-Toolchain-ramips-for-mipsel_24kec+dsp-gcc-4.8-linaro_uClibc-0.9.33.2
ENV TOOLCHAIN_SYSROOT=${TOOLCHAIN_ROOT}/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2
ENV TOOLCHAIN_EXES=${TOOLCHAIN_SYSROOT}/bin
ENV TOOLCHAIN_NAME=mipsel-openwrt-linux-uclibc
ENV AR=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-ar
ENV AS=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-as
ENV CC=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-gcc
ENV LD=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-ld
ENV NM=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-nm
ENV RANLIB=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-ranlib

ENV LDFLAGS="-L${TOOLCHAIN_SYSROOT}/usr/lib"
ENV LIBS="-lssl -lcrypto -ldl -lpthread"
ENV TOOLCHAIN_PREFIX=${TOOLCHAIN_SYSROOT}/usr
ENV STAGING_DIR=${TOOLCHAIN_SYSROOT}

# Build OpenSSL
WORKDIR openssl-1.0.2o
RUN ./Configure linux-generic32 shared --prefix=${TOOLCHAIN_PREFIX} --openssldir=${TOOLCHAIN_PREFIX}
RUN make
RUN make install
WORKDIR ..

# Build cURL
WORKDIR curl-7.60.0
RUN ./configure --with-sysroot=${TOOLCHAIN_SYSROOT} --prefix=${TOOLCHAIN_PREFIX} --target=${TOOLCHAIN_NAME} --with-ssl --with-zlib --host=${TOOLCHAIN_NAME} --build=x86_64-pc-linux-uclibc
RUN make
RUN make install
WORKDIR ..

# Build uuid
WORKDIR util-linux-2.32-rc2
RUN ./configure --prefix=${TOOLCHAIN_PREFIX} --with-sysroot=${TOOLCHAIN_SYSROOT} --target=${TOOLCHAIN_NAME} --host=${TOOLCHAIN_NAME} --disable-all-programs  --disable-bash-completion --enable-libuuid
RUN make
RUN make install
WORKDIR ..

# To build the SDK we need to create a cmake toolchain file. This tells cmake to use the tools in the
# toolchain rather than those on the host
WORKDIR azure-iot-sdk-c

# Create a working directory for the cmake operations
RUN mkdir cmake
WORKDIR cmake

# Create a cmake toolchain file on the fly
RUN echo "SET(CMAKE_SYSTEM_NAME Linux)     # this one is important" > toolchain.cmake
RUN echo "SET(CMAKE_SYSTEM_VERSION 1)      # this one not so much" >> toolchain.cmake
RUN echo "SET(CMAKE_SYSROOT ${TOOLCHAIN_SYSROOT})" >> toolchain.cmake
RUN echo "SET(CMAKE_C_COMPILER ${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-gcc)" >> toolchain.cmake
RUN echo "SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-g++)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH $ENV{TOOLCHAIN_SYSROOT})" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> toolchain.cmake
RUN echo "SET(set_trusted_cert_in_samples true CACHE BOOL \"Force use of TrustedCerts option\" FORCE)" >> toolchain.cmake

# Build the SDK. This will use the OpenSSL, cURL and uuid binaries that we built before
RUN cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -DCMAKE_INSTALL_PREFIX=${TOOLCHAIN_PREFIX} ..
RUN make
RUN make install

# Finally a sanity check to make sure the files are there
RUN ls -al ${TOOLCHAIN_PREFIX}/lib
RUN ls -al ${TOOLCHAIN_PREFIX}/include

# Go to project root
WORKDIR ../..
```

To run this script save it to a directory and change to that directory then simply enter:
```bash
docker build -t mipsiotbuild:latest . --network=host
```
You can replace the value 'mipsiotbuild' with any name that describes your build. In this instance it is building for a MIPS32 processor hence the name. Once the build is complete you will be ready to build your application against the libraries just built.

*Note:* The build of the Azure IoT C SDK uses cmake to generate the makefiles. In order to instruct cmake to perform a cross-compile one uses a cmake toolchain file. This tells cmake where to find the libraries etc. in the toolchain rather than on the host. This file is created on the fly in the above script to reduce dependencies on external files. Alternatively one could create this file outside the script and import into the container.

## Building Your Application
Once you have successfully built the SDK you are now ready to create your application. You will need to create a directory that contains your source code and a CMakeLists.txt. The application build can use the cmake toolchain file that is already present in the image. 

### Sample ```CMakeLists.txt``` for an Application
Here is a simple ```CMakeLists.txt``` file that demonstrates how to build an application that uses the libraries built above. This sample, for demonstration purposes uses a copy of the ```iothub_convenience_sample.c``` sample from the SDK.
```cmake
cmake_minimum_required(VERSION 2.8.11)
project(myapp_project)

# The demonstration uses C99 but it could just as easily be a C++ application
set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")

# Assume we will use the built in trusted certificates. 
# Many embedded devices will need this.
option(use_sample_trusted_cert "Set flag in samples to use SDK's built-in CA as TrustedCerts" ON)

set(iothub_c_files
    iothub_convenience_sample.c
)

# Conditionally use the SDK trusted certs in the samples (is set to true in cmake toolchain file)
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories($ENV{WORK_ROOT}/azure-iot-sdk-c/certs)
    set(iothub_c_files 
        ${iothub_c_files} 
        $ENV{WORK_ROOT}/azure-iot-sdk-c/certs/certs.c)
endif()

# Set up the include and library paths
include_directories($ENV{TOOLCHAIN_PREFIX}/include/)
include_directories($ENV{TOOLCHAIN_PREFIX}/include/azureiot)
link_directories($ENV{TOOLCHAIN_PREFIX}/lib)

add_executable(myapp ${iothub_c_files})

# Redundant in this case but demonstrates how to rename your output executable
set_target_properties(myapp PROPERTIES OUTPUT_NAME "myapp") 

# List the libraries required by the link step
target_link_libraries(myapp iothub_client_mqtt_transport iothub_client umqtt aziotsharedutil parson pthread curl ssl crypto m )
```
### Using a Separate Docker Script to Build the Application
Though one could append the application build steps to the Docker script above, this will demonstrate how to use the existing image and build the application in a seperate Docker script. This will keep the original image clean so it may be used for building multiple applications.

This sample script assumes you have your source files and the ```CMakeLists.txt``` in a directory called ```myapp```.
```docker
FROM mipsiotbuild:latest

USER builder
WORKDIR /home/builder

# Copy a directory from the host containing the files to build setting ownership at the same time
ADD --chown=builder:builder myapp myapp

# Sanity check
RUN ls -al myapp

# Switch to application directory
WORKDIR myapp

# Create and switch to cmake directory
RUN mkdir cmake
WORKDIR cmake

# Generate the makefiles with the same toolchain file and build
RUN cmake -DCMAKE_TOOLCHAIN_FILE=${WORK_ROOT}/azure-iot-sdk-c/cmake/toolchain.cmake ..
RUN make

# There should be an executable called myapp
RUN ls -al myapp
```

Execute this Docker script with:
```
docker build -t mipsiotapp:latest . --network=host
```
## Copying the Executable from the Docker Container
Now the applicaton is built the last step is to copy the new executable from your Docker container to your host so that it can be deployed to your target device. An example of how you might do this follows:

```
id =$(docker create mipsiotapp)
docker cp $id:/home/builder/myapp/cmake/myapp ./myapp_exe
docker rm -v $id
```
The above steps will create and acquire the Docker container identifier, copy the application to the local directory on the host and then delete the container.

**Note:** Depending upon your device you may need to copy additional binaries from the container in order to add them to your device. For example you device may not have the OpenSSL binaries so you will need to copy libssl.so and libcrypto.so. This could also be true for libuuid and libcurl. All of these libraries will be in the toolchain typically in ```/usr/local/lib```.
# A Complete Example
You can find two complete examples in [samples](../samples/dockerbuilds). This directory contains Docker scripts to cross compile the SDK for MIPS32 and for Raspbian and subsequently builds an application. In order to reduce the number of files required, the steps are slightly modified though they perform the same function. To cross compile the SDK for Raspbian, create an application and copy it to your host use the following steps. 
```bash
# Change directory to your Azure IoT SDK cloned repository root
cd <SDK Root>
# Work in this directory or two copies of the myapp directory will be required
cd samples/dockerbuilds
# Cross compile the SDK
docker build -t rpiiotbuild:latest ./RaspberryPi --network=host
# Build the application against the SDK
docker build -t rpiiotapp:latest . --network=host --file ./RaspberryPi/Dockerfile_adjunct
id=$(docker create rpiiotapp)
# Copy application to home directory
docker cp $id:/home/builder/myapp/cmake/myapp ~/myapp_rpi
docker rm -v $id
```
And for MIPS32:
```bash
# Change directory to your Azure IoT SDK cloned repository root
cd <SDK Root>
# Work in this directory or two copies of the myapp directory will be required
cd samples/dockerbuilds
# Cross compile the SDK
docker build -t mipsiotbuild:latest ./MIPS32 --network=host
# Build the application against the SDK
docker build -t mipsiotapp:latest . --network=host --file ./MIPS32/Dockerfile_adjunct
id=$(docker create mipsiotapp)
# Copy application to home directory
docker cp $id:/home/builder/myapp/cmake/myapp ~/myapp_mips
docker rm -v $id
```
**Note:** For these examples to work successfully the image names must be exactly as they are shown in the examples.
## Summing Up
This document demonstrates how to compile the Azure IoT SDK for C along with all of its dependents and then create and link an application with the libraries and headers. It uses a MIPS32 toolchain for this demonstration but it should be easily adaptable to the toolchain required by the target device.
