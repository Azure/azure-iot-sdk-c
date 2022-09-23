############################################################
# Dockerfile to Linux-c build container images
# Based on Ubuntu-20.04
############################################################

# Set the base image to Ubuntu
FROM ubuntu:20.04
################## BEGIN INSTALLATION ######################

# Install following tools for Linux-c-build
ARG DEBIAN_FRONTEND=noninteractive
#ENV TZ=America/Los_Angeles
#RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get install -y \
build-essential \
cmake \
curl \
gcc \
g++ \
git \
libcurl4-openssl-dev \
libssl-dev \
make \
openssh-server \
pkg-config \
uuid-dev \
valgrind 3.12 \`
wget \
softhsm2 \
opensc \
sudo \
dnsutils

RUN usermod -aG softhsm "$(id -un)"

WORKDIR /root
RUN git clone --branch libp11-0.4.11 https://github.com/OpenSC/libp11.git

WORKDIR /root/libp11
RUN ./bootstrap
RUN ./configure
RUN make -j
RUN make install

# Create the default data directory
RUN mkdir /var/run/sshd

# Install SoftHSM as default PKCS11 module (from https://github.com/OpenSC/libp11/blob/master/README.md#using-the-engine-from-the-command-line)
RUN echo "openssl_conf = openssl_init \n\n\
$(cat /etc/ssl/openssl.cnf) \n\n\
[openssl_init] \n\
engines=engine_section \n\n\
[engine_section] \n\
pkcs11 = pkcs11_section \n\n\
[pkcs11_section] \n\
engine_id = pkcs11 \n\
MODULE_PATH = /usr/lib/softhsm/libsofthsm2.so \n\
init = 0" > tmp.openssl.cnf

RUN cp tmp.openssl.cnf /etc/ssl/openssl.cnf
RUN cat /etc/ssl/openssl.cnf

WORKDIR /root
RUN echo "Testing OPENSSL + PKCS11 Engine:"
RUN openssl engine pkcs11 -t

# DevOps requires that the current folder is accessible by non-root users.
WORKDIR /
