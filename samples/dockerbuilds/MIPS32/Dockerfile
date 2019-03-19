FROM ubuntu:latest

# Run commands that require root authority
RUN apt-get update && apt-get -y upgrade
RUN apt-get install -y cmake git wget nano

RUN useradd -d /home/builder -ms /bin/bash -G sudo -p builder builder

# Switch to new user
USER builder
WORKDIR /home/builder

# Download all required files
RUN mkdir MIPSBuild
WORKDIR MIPSBuild

# Cross compile toolchain
RUN wget https://downloads.openwrt.org/barrier_breaker/14.07/ramips/mt7620n/OpenWrt-Toolchain-ramips-for-mipsel_24kec%2bdsp-gcc-4.8-linaro_uClibc-0.9.33.2.tar.bz2
RUN tar -xvf OpenWrt-Toolchain-ramips-for-mipsel_24kec+dsp-gcc-4.8-linaro_uClibc-0.9.33.2.tar.bz2

# Azure IoT SDK for C
RUN git clone --recursive https://github.com/azure/azure-iot-sdk-c.git

# OpenSSL
RUN wget https://www.openssl.org/source/openssl-1.0.2o.tar.gz
RUN tar -xvf openssl-1.0.2o.tar.gz

# Curl
RUN wget http://curl.haxx.se/download/curl-7.60.0.tar.gz
RUN tar -xvf curl-7.60.0.tar.gz

# Linux utilities for libuuid
RUN wget https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.32/util-linux-2.32-rc2.tar.gz
RUN tar -xvf util-linux-2.32-rc2.tar.gz

# Set up environment variables in preperation for the builds to follow
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

# Build curl
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

WORKDIR azure-iot-sdk-c
RUN mkdir cmake
WORKDIR cmake

# Create a toolchain file on the fly
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

RUN cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -DCMAKE_INSTALL_PREFIX=${TOOLCHAIN_PREFIX} ..
RUN make
RUN make install

WORKDIR ../..

CMD ["/bin/bash"]

