FROM ubuntu:latest
ARG DEBIAN_FRONTEND=noninteractive
ARG TARGET_BRANCH=main

#########################################
# start from home directory
RUN cd ~

#########################################
# Run commands that require root authority
RUN apt-get update && apt-get -y upgrade
RUN apt-get install -y make cmake git wget nano xz-utils bzip2

#########################################
# Create and switch to new user
RUN useradd -d /home/builder -ms /bin/bash -G sudo -p builder builder
USER builder
WORKDIR /home/builder
RUN mkdir ARMBuild
WORKDIR ARMBuild

#########################################
# Set up environment variables in preparation for the builds to follow
ENV WORK_ROOT=/home/builder/ARMBuild
ENV TOOLCHAIN_ARM=gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf
ENV TOOLCHAIN_PLATFORM=arm-none-linux-gnueabihf
ENV TOOLCHAIN_SYSROOT=${WORK_ROOT}/${TOOLCHAIN_ARM}
ENV TOOLCHAIN_BIN=${WORK_ROOT}/${TOOLCHAIN_ARM}/bin
ENV OPENSSL_ROOT_DIR=${WORK_ROOT}/openssl-1.1.1v
ENV TOOLCHAIN_PREFIX=${WORK_ROOT}/ARM
ENV AR=${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-ar
ENV CC=${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-gcc
ENV CXX=${TOOLCHAIN_BIN}/${TOOLCHAIN_PLATFORM}-g++

#########################################
# Download all required files
USER root

#########################################
# Download ARM GCC 10.3 with Linux ABI
RUN wget https://developer.arm.com/-/media/Files/downloads/gnu-a/10.3-2021.07/binrel/${TOOLCHAIN_ARM}.tar.xz
RUN tar -xvf ./${TOOLCHAIN_ARM}.tar.xz

#########################################
# Download and Configure OpenSSL
RUN wget https://github.com/openssl/openssl/releases/download/OpenSSL_1_1_1v/openssl-1.1.1v.tar.gz 
RUN tar -xvf ./openssl-1.1.1v.tar.gz 
WORKDIR openssl-1.1.1v 
RUN ./Configure linux-armv4 --prefix=${TOOLCHAIN_PREFIX} --openssldir=${OPENSSL_ROOT_DIR} no-tests shared 
RUN make 
RUN make install_sw
WORKDIR ..

#########################################
# Build curl
RUN wget http://curl.haxx.se/download/curl-7.60.0.tar.gz 
RUN tar -xvf curl-7.60.0.tar.gz 
WORKDIR curl-7.60.0 
RUN ./configure --with-sysroot=${TOOLCHAIN_SYSROOT} --prefix=${TOOLCHAIN_PREFIX} --target=${TOOLCHAIN_PLATFORM} --with-ssl --with-zlib --host=${TOOLCHAIN_PLATFORM} 
RUN make 
RUN make install 
WORKDIR ..

#########################################
# Build uuid
RUN wget https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.32/util-linux-2.32-rc2.tar.gz 
RUN tar -xvf util-linux-2.32-rc2.tar.gz 
WORKDIR util-linux-2.32-rc2/ 
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

# Create a CMAKE toolchain file on the fly
RUN echo "SET(CMAKE_SYSTEM_NAME Linux)  # this one is important" > toolchain.cmake
RUN echo "SET(CMAKE_SYSTEM_VERSION 1)" >> toolchain.cmake
RUN echo "SET(CMAKE_C_COMPILER ${CC})" >> toolchain.cmake
RUN echo "SET(CMAKE_CXX_COMPILER ${CXX})" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH ${WORK_ROOT})" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> toolchain.cmake
RUN echo "SET(CURL_LIBRARIES ${TOOLCHAIN_PREFIX}/lib/libcurl.so)" >> toolchain.cmake
RUN echo "SET(ENV{LDFLAGS} -L${TOOLCHAIN_PREFIX}/lib)" >> toolchain.cmake
RUN echo "SET(set_trusted_cert_in_samples true CACHE BOOL \"Force use of TrustedCerts option\" FORCE)" >> toolchain.cmake
RUN echo "include_directories(${TOOLCHAIN_PREFIX}/include)" >> toolchain.cmake

RUN cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake ..
RUN cmake --build .
RUN cmake --install . --prefix ${TOOLCHAIN_PREFIX}

WORKDIR ../..

CMD ["/bin/bash"]
