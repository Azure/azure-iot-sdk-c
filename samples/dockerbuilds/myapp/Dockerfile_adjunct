# This is a sample - there are working versions specifically for Raspberry Pi and MIPS32

# This file could be appended to the Dockerfile or could be set up to use the image created by 
# the build step by including:

# FROM <your image name>:latest
# USER builder
# WORKDIR /home/builder

# Copy a directory from the host containing the files to build setting ownership at the same time
ADD --chown=builder:builder myapp  ${WORK_ROOT}/myapp

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
