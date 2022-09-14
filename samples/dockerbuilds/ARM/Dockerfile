FROM ubuntu:latest

ARG DEBIAN_FRONTEND=noninteractive

# Run commands that require root authority
#########################################
# start from home directory
RUN cd ~

RUN apt-get update && apt-get -y upgrade
RUN apt-get install -y make cmake git wget nano xz-utils bzip2

RUN useradd -d /home/builder -ms /bin/bash -G sudo -p builder builder

# Switch to new user
USER builder
WORKDIR /home/builder

# Download all required files
RUN mkdir ARMBuild
WORKDIR ARMBuild

USER root
 
# Set up environment variables in preparation for the builds to follow
ENV WORK_ROOT=/home/builder/ARMBuild
ENV TOOLCHAIN_ARM=gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf
ENV TOOLCHAIN_PLATFORM=arm-none-linux-gnueabihf
ENV TOOLCHAIN_SYSROOT=${WORK_ROOT}/${TOOLCHAIN_ARM}
ENV TOOLCHAIN_BIN=${WORK_ROOT}/${TOOLCHAIN_ARM}/bin
ENV OPENSSL_ROOT_DIR=${WORK_ROOT}/openssl-OpenSSL_1_1_1f
ENV TOOLCHAIN_PREFIX=${TOOLCHAIN_SYSROOT}/usr

#########################################
# Download ARM GCC 10.3 with Linux ABI

RUN wget https://developer.arm.com/-/media/Files/downloads/gnu-a/10.3-2021.07/binrel/${TOOLCHAIN_ARM}.tar.xz
RUN tar -xvf ./${TOOLCHAIN_ARM}.tar.xz

#########################################
# Download and Configure OpenSSL

RUN wget https://github.com/openssl/openssl/archive/refs/tags/OpenSSL_1_1_1f.tar.gz \
&& tar -xvf ./OpenSSL_1_1_1f.tar.gz \
&& cd openssl-OpenSSL_1_1_1f \
&& ./Configure linux-armv4 --prefix=${TOOLCHAIN_SYSROOT}/bin --openssldir=${OPENSSL_ROOT_DIR} shared \
&& make CC=${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-gcc AR=${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-ar \
&& make install \
&& cd ..

#########################################
# Build curl

RUN wget http://curl.haxx.se/download/curl-7.60.0.tar.gz \
&& tar -xvf curl-7.60.0.tar.gz \
&& cd curl-7.60.0 \
&& ./configure --with-sysroot=${TOOLCHAIN_SYSROOT} --prefix=${TOOLCHAIN_SYSROOT}/${TOOLCHAIN_PLATFORM}/libc --target=${TOOLCHAIN_PLATFORM} --with-ssl --with-zlib --host=${TOOLCHAIN_PLATFORM} CC=${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-gcc AR=${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-ar \
&& make \
&& make install \
&& cd ..

#########################################
#Build uuid

RUN wget https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.32/util-linux-2.32-rc2.tar.gz \
&& tar -xvf util-linux-2.32-rc2.tar.gz \
&& cd util-linux-2.32-rc2/ \
&& ./configure --prefix=${TOOLCHAIN_PREFIX} --with-sysroot=${TOOLCHAIN_SYSROOT} --target=${TOOLCHAIN_NAME} --host=${TOOLCHAIN_NAME} --disable-all-programs  --disable-bash-completion --enable-libuuid \
&& make \
&& make install \
&& cd ..

#########################################
# Build Azure C SDK

RUN git clone --recursive https://github.com/azure/azure-iot-sdk-c.git
WORKDIR azure-iot-sdk-c
RUN git checkout dane/arm
RUN mkdir cmake
WORKDIR cmake

# Create a toolchain file on the fly
RUN echo "SET(CMAKE_SYSTEM_NAME Linux)     # this one is important" > toolchain.cmake
RUN echo "SET(CMAKE_SYSTEM_VERSION 1)      # this one not so much" >> toolchain.cmake
RUN echo "SET(CMAKE_SYSROOT ${TOOLCHAIN_SYSROOT}/${TOOLCHAIN_PLATFORM}/libc)" >> toolchain.cmake
RUN echo "SET(CMAKE_C_COMPILER ${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-gcc)" >> toolchain.cmake
RUN echo "SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-g++)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_SYSROOT}/${TOOLCHAIN_PLATFORM})" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> toolchain.cmake
RUN echo "SET(OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR})" >> toolchain.cmake
RUN echo "SET(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_ROOT_DIR}/libcrypto.so.1.1)" >> toolchain.cmake
RUN echo "SET(OPENSSL_SSL_LIBRARY ${OPENSSL_ROOT_DIR}/libssl.so.1.1)" >> toolchain.cmake
RUN echo "SET(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include)" >> toolchain.cmake
RUN cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -Duse_default_uuid=ON -Duse_condition=OFF ..
RUN make
RUN make install

WORKDIR ../..

CMD ["/bin/bash"]
