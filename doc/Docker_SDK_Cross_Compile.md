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

#########################################
# start from home directory
RUN cd ~

#########################################
# Run commands that require root authority
RUN apt-get update && apt-get -y upgrade
RUN apt-get install -y cmake git wget nano xz-utils bzip2

#########################################
# Create and switch to new user
RUN useradd -d /home/builder -ms /bin/bash -G sudo -p builder builder
USER builder
WORKDIR /home/builder

#########################################
# Download all required files
RUN mkdir MIPSBuild
WORKDIR MIPSBuild

# Cross compile toolchain
RUN wget https://downloads.openwrt.org/releases/21.02.3/targets/ramips/mt7620/openwrt-sdk-21.02.3-ramips-mt7620_gcc-8.4.0_musl.Linux-x86_64.tar.xz
RUN tar -xvf openwrt-sdk-21.02.3-ramips-mt7620_gcc-8.4.0_musl.Linux-x86_64.tar.xz

# OpenSSL
RUN wget https://www.openssl.org/source/openssl-1.0.2o.tar.gz
RUN tar -xvf openssl-1.0.2o.tar.gz

# Curl
RUN wget http://curl.haxx.se/download/curl-7.60.0.tar.gz
RUN tar -xvf curl-7.60.0.tar.gz

# Linux utilities for libuuid
RUN wget https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.32/util-linux-2.32-rc2.tar.gz
RUN tar -xvf util-linux-2.32-rc2.tar.gz

#########################################
# Set up environment variables in preperation for the builds to follow
ENV WORK_ROOT=/home/builder/MIPSBuild
ENV TOOLCHAIN_MIPS=openwrt-sdk-21.02.3-ramips-mt7620_gcc-8.4.0_musl.Linux-x86_64
ENV TOOLCHAIN_PLATFORM=mipsel-openwrt-linux-musl
ENV STAGING_DIR=${WORK_ROOT}/${TOOLCHAIN_MIPS}/staging_dir
ENV TOOLCHAIN_SYSROOT=${WORK_ROOT}/${TOOLCHAIN_MIPS}/staging_dir/toolchain-mipsel_24kc_gcc-8.4.0_musl
ENV TOOLCHAIN_BIN=${TOOLCHAIN_SYSROOT}/bin
ENV OPENSSL_ROOT_DIR=${WORK_ROOT}/openssl-OpenSSL_1_1_1f
ENV TOOLCHAIN_PREFIX=${WORK_ROOT}/MIPS
ENV AR=${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-ar
ENV CC=${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-gcc
ENV CXX=${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-g++


ENV LDFLAGS="-L${TOOLCHAIN_PREFIX}/lib"
ENV LIBS="-lssl -lcrypto -ldl -lpthread"

# Build OpenSSL
WORKDIR openssl-1.0.2o
RUN ./Configure linux-generic32 --prefix=${TOOLCHAIN_PREFIX} --openssldir=${OPENSSL_ROOT_DIR} no-tests shared 
RUN make
RUN make install_sw
WORKDIR ..

# Build curl
WORKDIR curl-7.60.0
RUN ./configure --with-sysroot=${TOOLCHAIN_SYSROOT} --prefix=${TOOLCHAIN_PREFIX} --target=${TOOLCHAIN_PLATFORM} --with-ssl=${TOOLCHAIN_PREFIX} --with-zlib --host=${TOOLCHAIN_PLATFORM} 
RUN make
RUN make install
WORKDIR ..

# Build uuid
WORKDIR util-linux-2.32-rc2
RUN ./configure --with-sysroot=${TOOLCHAIN_SYSROOT} --prefix=${TOOLCHAIN_PREFIX} --target=${TOOLCHAIN_PLATFORM} --host=${TOOLCHAIN_PLATFORM} --disable-all-programs  --disable-bash-completion --enable-libuuid
RUN make
RUN make install
WORKDIR ..

#########################################
# Build Azure C SDK

RUN git clone https://github.com/azure/azure-iot-sdk-c.git
WORKDIR azure-iot-sdk-c
RUN git submodule update --init
RUN mkdir cmake
WORKDIR cmake

# Create a toolchain file on the fly
RUN echo "SET(CMAKE_SYSTEM_NAME Linux)     # this one is important" > toolchain.cmake
RUN echo "SET(CMAKE_SYSTEM_VERSION 1)"  >> toolchain.cmake
RUN echo "SET(CMAKE_SYSROOT ${TOOLCHAIN_SYSROOT})" >> toolchain.cmake
RUN echo "SET(CMAKE_C_COMPILER ${CC})" >> toolchain.cmake
RUN echo "SET(CMAKE_CXX_COMPILER ${CXX})" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH ${WORK_ROOT})" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> toolchain.cmake
RUN echo "SET(CURL_LIBRARIES ${TOOLCHAIN_PREFIX}/lib/libcurl.so)" >> toolchain.cmake
RUN echo "SET(ENV{LDFLAGS} -L${TOOLCHAIN_PREFIX}/lib)" >> toolchain.cmake
RUN echo "SET(OPENSSL_ROOT_DIR ${TOOLCHAIN_PREFIX})" >> toolchain.cmake
RUN echo "SET(set_trusted_cert_in_samples true CACHE BOOL \"Force use of TrustedCerts option\" FORCE)" >> toolchain.cmake
RUN echo "include_directories(${TOOLCHAIN_PREFIX}/include)" >> toolchain.cmake

RUN cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake ..
RUN cmake --build .
RUN cmake --install . --prefix ${TOOLCHAIN_PREFIX}

WORKDIR ../..

CMD ["/bin/bash"]

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
Here is a simple ```CMakeLists.txt``` file that demonstrates how to build an application that uses the libraries built above. This sample, for demonstration purposes uses a copy of the ```iothub_cross_compile_simple_sample.c``` sample from the SDK.
```cmake
cmake_minimum_required (VERSION 3.5)
project(myapp_project)

# The demonstration uses C99 but it could just as easily be a C++ application
set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")

# Assume we will use the built in trusted certificates. 
# Many embedded devices will need this.
option(use_sample_trusted_cert "Set flag in samples to use SDK's built-in CA as TrustedCerts" ON)

set(iothub_c_files
    iothub_cross_compile_simple_sample.c
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
include_directories(${CMAKE_INSTALL_PREFIX}/include/)
include_directories(${CMAKE_INSTALL_PREFIX}/include/azureiot)
link_directories(/usr/local/lib)
link_directories($ENV{TOOLCHAIN_PREFIX}/lib)

add_executable(myapp ${iothub_c_files})

# Redundant in this case but shows how to rename your output executable
set_target_properties(myapp PROPERTIES OUTPUT_NAME "myapp")

# If OpenSSL::SSL OR OpenSSL::Crypto are not set then you need to run
# the find package for openssl
if (NOT TARGET OpenSSL::SSL OR NOT TARGET OpenSSL::Crypto OR NOT ${OPENSSL_INCLUDE_DIR})
    find_package(OpenSSL REQUIRED)
endif()
include_directories(${OPENSSL_INCLUDE_DIR})

# List the libraries required by the link step
target_link_libraries(myapp iothub_client prov_device_client iothub_client_mqtt_transport prov_auth_client umqtt hsm_security_client utpm dl aziotsharedutil parson pthread curl OpenSSL::SSL OpenSSL::Crypto m )
```
### Using a Separate Docker Script to Build the Application
Though one could append the application build steps to the Docker script above, this will demonstrate how to use the existing image and build the application in a seperate Docker script. This will keep the original image clean so it may be used for building multiple applications.

This sample script assumes you have your source files and the ```CMakeLists.txt``` in a directory called ```myapp```.
```docker
FROM mipsiotbuild:latest

ENV WORK_ROOT=/home/builder/MIPSBuild
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
You can find three complete examples in [samples](../samples/dockerbuilds). This directory contains Docker scripts to cross compile the SDK for MIPS32, ARM, and for Raspbian and subsequently builds an application. In order to reduce the number of files required, the steps are slightly modified though they perform the same function. To cross compile the SDK for Raspbian, create an application and copy it to your host use the following steps. 
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
For MIPS32:
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
And for ARM:
```bash
# Change directory to your Azure IoT SDK cloned repository root
cd <SDK Root>
# Work in this directory or two copies of the myapp directory will be required
cd samples/dockerbuilds
# Cross compile the SDK
docker build -t armiotbuild:latest ./ARM --network=host
# Build the application against the SDK
docker build -t armiotapp:latest . --network=host --file ./ARM/Dockerfile_adjunct
id=$(docker create armiotapp)
# Copy application to home directory
docker cp $id:/home/builder/myapp/cmake/myapp ~/myapp_arm
docker rm -v $id
```
**Note:** For these examples to work successfully the image names must be exactly as they are shown in the examples.
## Summing Up
This document demonstrates how to compile the Azure IoT SDK for C along with all of its dependents and then create and link an application with the libraries and headers. It uses a MIPS32 toolchain for this demonstration but it should be easily adaptable to the toolchain required by the target device.
