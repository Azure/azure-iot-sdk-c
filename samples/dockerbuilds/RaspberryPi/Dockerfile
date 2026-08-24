# Start with a modern Debian base for cross-compilation
FROM debian:bookworm

RUN ls -la

# Fetch and install all outstanding updates
RUN apt-get update && apt-get -y upgrade

# Install wget git cmake xz-utils
RUN apt-get install -y wget git cmake xz-utils ca-certificates pkg-config build-essential

# Add a non-root user
RUN useradd -d /home/builder -ms /bin/bash -G sudo -p builder builder

# Switch to new user
USER builder
WORKDIR /home/builder
#WORKDIR /root

# Don't use RPiTools because gcc is old, use Arm GNU Toolchain (GCC 13.3)
#RUN mkdir RPiTools
#WORKDIR RPiTools
#RUN git clone https://github.com/raspberrypi/tools.git

RUN mkdir RPiBuild
ENV WORK_ROOT=/home/builder/RPiBuild
WORKDIR ${WORK_ROOT}

RUN wget https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz
RUN tar -xvf arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz

# Set up environment variables for builds
ENV TOOLCHAIN_ROOT=${WORK_ROOT}/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-linux-gnueabihf
ENV TOOLCHAIN_SYSROOT=${TOOLCHAIN_ROOT}
ENV TOOLCHAIN_EXES=${TOOLCHAIN_SYSROOT}/bin
ENV TOOLCHAIN_NAME=arm-none-linux-gnueabihf
ENV TOOLCHAIN_PREFIX=${TOOLCHAIN_SYSROOT}/usr

ENV AR=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-ar
ENV AS=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-as
ENV CC=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-gcc
ENV LD=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-ld
ENV NM=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-nm

ENV LDFLAGS="-L${TOOLCHAIN_SYSROOT}/usr/lib"
ENV LIBS="-lssl -lcrypto -ldl -lpthread -latomic"
ENV TOOLCHAIN_PREFIX=${TOOLCHAIN_SYSROOT}/usr
ENV STAGING_DIR=${TOOLCHAIN_SYSROOT}

# Download OpenSSL source and expand it
RUN wget https://www.openssl.org/source/openssl-3.0.15.tar.gz
RUN tar -xvf openssl-3.0.15.tar.gz

# Build OpenSSL
WORKDIR openssl-3.0.15
RUN ./Configure linux-generic32 shared no-tests --prefix=${TOOLCHAIN_PREFIX} --openssldir=${TOOLCHAIN_PREFIX}
RUN make
RUN make install
WORKDIR ..

# Download cURL source and expand it
ENV CURL_VERSION=8.20.0
RUN wget https://curl.se/download/curl-${CURL_VERSION}.tar.gz
RUN tar -xvf curl-${CURL_VERSION}.tar.gz

# Build cURL
# --with-openssl points at our cross-built OpenSSL (option renamed from --with-ssl in curl 7.77.0).
# --disable-ntlm excludes curl's MD4-based NTLM code (banned hash algorithm MD4 finding).
WORKDIR curl-${CURL_VERSION}
RUN ./configure --with-sysroot=${TOOLCHAIN_SYSROOT} --prefix=${TOOLCHAIN_PREFIX} --target=${TOOLCHAIN_NAME} --with-openssl=${TOOLCHAIN_PREFIX} --with-zlib --host=${TOOLCHAIN_NAME} --build=x86_64-linux-gnu --without-libpsl --disable-ntlm
RUN make
RUN make install
WORKDIR ..

# Download the Linux utilities for libuuid and expand it
RUN wget https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.32/util-linux-2.32-rc2.tar.gz
RUN tar -xvf util-linux-2.32-rc2.tar.gz

# Build uuid
WORKDIR util-linux-2.32-rc2
RUN ./configure --prefix=${TOOLCHAIN_PREFIX} --with-sysroot=${TOOLCHAIN_SYSROOT} --target=${TOOLCHAIN_NAME} --host=${TOOLCHAIN_NAME} --disable-all-programs  --disable-bash-completion --enable-libuuid
RUN make
RUN make install
WORKDIR ..

# clone azure 
RUN git clone --recursive https://github.com/Azure/azure-iot-sdk-c.git
WORKDIR azure-iot-sdk-c

# Create a working directory for the cmake operations
RUN mkdir cmake
WORKDIR cmake

# Create a cmake toolchain file on the fly
RUN echo "SET(CMAKE_SYSTEM_NAME Linux)     # this one is important" > toolchain.cmake
RUN echo "SET(CMAKE_SYSTEM_VERSION 1)      # this one not so much" >> toolchain.cmake

RUN echo "SET(CMAKE_C_COMPILER ${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-gcc)" >> toolchain.cmake
RUN echo "SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-g++)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_SYSROOT})" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> toolchain.cmake
RUN echo "SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> toolchain.cmake

# Build the SDK. This will use the OpenSSL, cURL and uuid binaries that we built before
RUN cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -Duse_prov_client:BOOL=OFF -DCMAKE_INSTALL_PREFIX=${TOOLCHAIN_PREFIX} ..
RUN make -j 2
RUN make install
# or RUN cmake --build .

WORKDIR ../..

CMD ["/bin/bash"]
