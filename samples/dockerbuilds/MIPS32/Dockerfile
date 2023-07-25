FROM ubuntu:latest
ARG TARGET_BRANCH=main

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
RUN git fetch origin $TARGET_BRANCH
RUN git checkout FETCH_HEAD
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

