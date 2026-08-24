#!/bin/bash

set -e

sudo apt-get update -qq
sudo apt install --fix-missing -y wget git build-essential cmake xz-utils ca-certificates pkg-config sudo

export WORK_ROOT="$(pwd)/toolchain"
mkdir $WORK_ROOT && pushd $WORK_ROOT

# ARM GNU TOOLCHAIN INSTALL (GCC 13.3, replaces discontinued Linaro 7.x)
export TOOLCHAIN_SOURCE=arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-linux-gnueabihf
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/${TOOLCHAIN_SOURCE}.tar.xz
tar -xvf ${TOOLCHAIN_SOURCE}.tar.xz

# Set up environment variables for builds
export TOOLCHAIN_ROOT=${WORK_ROOT}/${TOOLCHAIN_SOURCE}
export TOOLCHAIN_SYSROOT=${TOOLCHAIN_ROOT}
export TOOLCHAIN_EXES=${TOOLCHAIN_SYSROOT}/bin
export TOOLCHAIN_NAME=arm-none-linux-gnueabihf
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
# OpenSSL 3.0.15: curl 8.20.0's configure requires OpenSSL >= 3.0.0.
export OPENSSL_SOURCE=openssl-3.0.15
wget https://www.openssl.org/source/${OPENSSL_SOURCE}.tar.gz
tar -xvf ${OPENSSL_SOURCE}.tar.gz

# Build OpenSSL
cd ${WORK_ROOT}/${OPENSSL_SOURCE}
./Configure linux-generic32 shared no-tests --prefix=${TOOLCHAIN_PREFIX} --openssldir=${TOOLCHAIN_PREFIX}
make
make install
cd ${WORK_ROOT}


# CURL INSTALL
# Download cURL source and expand it
export CURL_SOURCE=curl-8.20.0
wget https://curl.se/download/${CURL_SOURCE}.tar.gz
tar -xvf ${CURL_SOURCE}.tar.gz

# Build cURL
# --with-openssl points at our cross-built OpenSSL (option renamed from --with-ssl in curl 7.77.0).
# --without-libpsl avoids curl 8.x's hard libpsl dependency, which is not in the sysroot.
# --disable-ntlm excludes curl's MD4/DES-based NTLM code (curl_ntlm_core.c), flagged as use of
# the banned hash algorithm MD4 and the banned symmetric algorithm DES.
pushd /${WORK_ROOT}/${CURL_SOURCE}
./configure --with-sysroot=${TOOLCHAIN_SYSROOT} --prefix=${TOOLCHAIN_PREFIX} --target=${TOOLCHAIN_NAME} --with-openssl=${TOOLCHAIN_PREFIX} --with-zlib --host=${TOOLCHAIN_NAME} --build=x86_64-linux-gnu --without-libpsl --disable-ntlm

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

cat > ./raspberry_env_vars.sh << EOF
export TOOLCHAIN_ROOT="$TOOLCHAIN_ROOT"
export TOOLCHAIN_SYSROOT="$TOOLCHAIN_SYSROOT"
export TOOLCHAIN_EXES="$TOOLCHAIN_EXES"
export TOOLCHAIN_NAME=arm-none-linux-gnueabihf
export TOOLCHAIN_PREFIX="$TOOLCHAIN_PREFIX"
EOF
