# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

# To run locally (example)
# docker build -t rpiiotbuild:latest --build-arg "CLIENTLIBRARY_REPO=Azure/azure-iot-sdk-c" --build-arg "CLIENTLIBRARY_COMMIT_SHA=raspi-pipeline" . --network=host --file ./Dockerfile


# Start with the latest version of the Debian Docker container
FROM debian:stretch

# Fetch and install all outstanding updates
RUN apt update && apt -y upgrade

# Install wget git cmake xz-utils
RUN apt install --fix-missing -y wget git build-essential cmake xz-utils ca-certificates pkg-config uuid-dev

ENV WORK_ROOT=/toolchain
WORKDIR ${WORK_ROOT}


########## LINARO INSTALL ##########
ENV LINARO_SOURCE=gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf
RUN wget https://releases.linaro.org/components/toolchain/binaries/7.4-2019.02/arm-linux-gnueabihf/${LINARO_SOURCE}.tar.xz
RUN tar -xvf ${LINARO_SOURCE}.tar.xz

# Set up environment variables for builds

ENV TOOLCHAIN_ROOT=${WORK_ROOT}/${LINARO_SOURCE}
ENV TOOLCHAIN_SYSROOT=${TOOLCHAIN_ROOT}
ENV TOOLCHAIN_EXES=${TOOLCHAIN_SYSROOT}/bin
ENV TOOLCHAIN_NAME=arm-linux-gnueabihf
ENV TOOLCHAIN_PREFIX=${TOOLCHAIN_SYSROOT}/usr

ENV AR=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-ar
ENV AS=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-as
ENV CC=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-gcc
ENV LD=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-ld
ENV NM=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-nm

ENV LDFLAGS="-L${TOOLCHAIN_SYSROOT}/usr/lib"
ENV LIBS="-lssl -lcrypto -ldl -lpthread"
ENV STAGING_DIR=${TOOLCHAIN_SYSROOT}


########## OPENSSL INSTALL ##########
# Download OpenSSL source and expand it
ENV OPENSSL_SOURCE=openssl-1.1.1f
RUN wget https://www.openssl.org/source/${OPENSSL_SOURCE}.tar.gz
RUN tar -xvf ${OPENSSL_SOURCE}.tar.gz

# Build OpenSSL

WORKDIR /${WORK_ROOT}/${OPENSSL_SOURCE}
RUN ./Configure linux-generic32 shared --prefix=${TOOLCHAIN_PREFIX} --openssldir=${TOOLCHAIN_PREFIX}
RUN make
RUN make install
WORKDIR /${WORK_ROOT}


########## CURL INSTALL ##########
# Download cURL source and expand it
ENV CURL_SOURCE=curl-7.64.1
RUN wget http://curl.haxx.se/download/${CURL_SOURCE}.tar.gz
RUN tar -xvf ${CURL_SOURCE}.tar.gz

# Build cURL
# we need to set the path for openssl with --with-ssl=...
WORKDIR /${WORK_ROOT}/${CURL_SOURCE}
RUN ./configure --with-sysroot=${TOOLCHAIN_SYSROOT} --prefix=${TOOLCHAIN_PREFIX} --target=${TOOLCHAIN_NAME} --with-ssl=${TOOLCHAIN_PREFIX} --with-zlib --host=${TOOLCHAIN_NAME} --build=x86_64-linux-gnu

RUN make
RUN make install
WORKDIR /${WORK_ROOT}


########## UTIL LINUX INSTALL ##########
# Download the Linux utilities for libuuid and expand it
ENV UTIL_LINUX_SOURCE=util-linux-2.33-rc2
RUN wget https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.33/${UTIL_LINUX_SOURCE}.tar.gz
RUN tar -xvf ${UTIL_LINUX_SOURCE}.tar.gz

# Build uuid
WORKDIR /${WORK_ROOT}/${UTIL_LINUX_SOURCE}
RUN ./configure --prefix=${TOOLCHAIN_PREFIX} --with-sysroot=${TOOLCHAIN_SYSROOT} --target=${TOOLCHAIN_NAME} --host=${TOOLCHAIN_NAME} --disable-all-programs  --disable-bash-completion --enable-libuuid
RUN make
RUN make install
WORKDIR /${WORK_ROOT}


########## CLIENT LIBRARY INSTALL ##########
# clone azure
ARG CLIENTLIBRARY_REPO
WORKDIR /sdk
RUN git clone https://github.com/$CLIENTLIBRARY_REPO .
RUN git submodule update --init

RUN mkdir cmake
WORKDIR /sdk/cmake
RUN ls -al

# Create a cmake toolchain file on the fly
RUN echo "SET(CMAKE_SYSTEM_NAME Linux)     # this one is important" > toolchain.cmake
RUN echo "SET(CMAKE_SYSTEM_VERSION 1)      # this one not so much" >> toolchain.cmake

RUN echo "SET(CMAKE_C_COMPILER ${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-gcc)" >> toolchain.cmake
RUN echo "SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-g++)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_SYSROOT})" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> toolchain.cmake
RUN ls -al

# Build the SDK. This will use the OpenSSL, cURL and uuid binaries that we built before
RUN cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -Duse_prov_client:BOOL=OFF -DCMAKE_INSTALL_PREFIX=${TOOLCHAIN_PREFIX} -Drun_e2e_tests:BOOL=ON -Drun_unittests=:BOOL=ON ..
RUN make -j 2
RUN ls -al

########## PHASE 2: COMPILE BRANCH SPECIFIC INFORMATION ##########
ARG CLIENTLIBRARY_COMMIT_SHA
ARG CLIENTLIBRARY_COMMIT_NAME
RUN echo "$CLIENTLIBRARY_COMMIT_NAME"
RUN echo "$CLIENTLIBRARY_COMMIT_SHA"

WORKDIR /sdk
RUN git pull
RUN git checkout $CLIENTLIBRARY_COMMIT_SHA

COPY ./patchfile /
# our base image might have some files checked out.  revert these.
RUN git reset HEAD && git checkout && git clean  -df
RUN if [ -s /patchfile ]; then git apply --index /patchfile; fi

RUN git submodule update --init

WORKDIR /sdk/cmake 
RUN if [ -f "CMakeCache.txt" ]; then rm CMakeCache.txt; fi
RUN cmake ..
RUN make -j 2

# Finally a sanity check to make sure the files are there
RUN ls -al ${TOOLCHAIN_PREFIX}/lib
RUN ls -al ${TOOLCHAIN_PREFIX}/include

RUN ls -la ./

# Go to project root
WORKDIR /
