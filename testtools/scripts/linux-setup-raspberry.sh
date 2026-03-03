#!/bin/bash

set -e

sudo apt-get update && sudo apt-get upgrade
sudo apt install --fix-missing -y wget git build-essential cmake xz-utils ca-certificates pkg-config uuid-dev sudo

export WORK_ROOT="$(pwd)/toolchain"
mkdir $WORK_ROOT && pushd $WORK_ROOT

# LINARO INSTALL
export LINARO_SOURCE=gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf
wget https://releases.linaro.org/components/toolchain/binaries/7.4-2019.02/arm-linux-gnueabihf/${LINARO_SOURCE}.tar.xz
tar -xvf ${LINARO_SOURCE}.tar.xz

# Set up environment variables for builds
export TOOLCHAIN_ROOT=${WORK_ROOT}/${LINARO_SOURCE}
export TOOLCHAIN_SYSROOT=${TOOLCHAIN_ROOT}
export TOOLCHAIN_EXES=${TOOLCHAIN_SYSROOT}/bin
export TOOLCHAIN_NAME=arm-linux-gnueabihf
export TOOLCHAIN_PREFIX=${TOOLCHAIN_SYSROOT}/usr

export AR=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-ar
export AS=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-as
export CC=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-gcc
export LD=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-ld
export NM=${TOOLCHAIN_EXES}/${TOOLCHAIN_NAME}-nm

export LDFLAGS="-L${TOOLCHAIN_SYSROOT}/usr/lib"
export LIBS="-lssl -lcrypto -ldl -lpthread"
export STAGING_DIR=${TOOLCHAIN_SYSROOT}


# OPENSSL INSTALL
# Download OpenSSL source and expand it
export OPENSSL_SOURCE=openssl-1.1.1f
wget https://www.openssl.org/source/${OPENSSL_SOURCE}.tar.gz
tar -xvf ${OPENSSL_SOURCE}.tar.gz

# Build OpenSSL
cd ${WORK_ROOT}/${OPENSSL_SOURCE}
./Configure linux-generic32 shared --prefix=${TOOLCHAIN_PREFIX} --openssldir=${TOOLCHAIN_PREFIX}
make
make install
cd ${WORK_ROOT}


# CURL INSTALL
# Download cURL source and expand it
export CURL_SOURCE=curl-7.64.1
wget http://curl.haxx.se/download/${CURL_SOURCE}.tar.gz
tar -xvf ${CURL_SOURCE}.tar.gz

# Build cURL
# we need to set the path for openssl with --with-ssl=...
pushd /${WORK_ROOT}/${CURL_SOURCE}
./configure --with-sysroot=${TOOLCHAIN_SYSROOT} --prefix=${TOOLCHAIN_PREFIX} --target=${TOOLCHAIN_NAME} --with-ssl=${TOOLCHAIN_PREFIX} --with-zlib --host=${TOOLCHAIN_NAME} --build=x86_64-linux-gnu

make
make install
cd ${WORK_ROOT}

########## UTIL LINUX INSTALL ##########
# Download the Linux utilities for libuuid and expand it
export UTIL_LINUX_SOURCE=util-linux-2.33-rc2
wget https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.33/${UTIL_LINUX_SOURCE}.tar.gz
tar -xvf ${UTIL_LINUX_SOURCE}.tar.gz

# Build uuid
cd ${WORK_ROOT}/${UTIL_LINUX_SOURCE}
./configure --prefix=${TOOLCHAIN_PREFIX} --with-sysroot=${TOOLCHAIN_SYSROOT} --target=${TOOLCHAIN_NAME} --host=${TOOLCHAIN_NAME} --disable-all-programs  --disable-bash-completion --enable-libuuid
make
make install
cd ${WORK_ROOT}

# Finally a sanity check to make sure the files are there
ls -al ${TOOLCHAIN_PREFIX}/lib
ls -al ${TOOLCHAIN_PREFIX}/include

ls -la ./
