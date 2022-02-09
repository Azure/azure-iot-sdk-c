# SDK CMake integration with your application

This document describes how to use your application CMake configuration to include and build the device C SDK.

## C Device SDK Git submodule
Inclusion of the device C SDK usually uses git submodules to include the SDK into your application source code. For the example project CMake file below, the device C SDK is installed at your root project in a directory called `azure-iot-sdk-c`.

## Application CMake project example
The device C SDK CMake files are configured to be directly referenced from your application CMake. The first step of your application CMake file is to set the SDK configuration options, and then include the SDK in your CMake by adding the `add_subdirectory()` to the SDK location within your project.

Below is an example of an application project CMake.txt file which referances the Azure IoT device C SDK source code.
```Shell
cmake_minimum_required (VERSION 3.7)
PROJECT(iothub_device_sample_project C)

# Set Azure IoT SDK C settings
set(use_mqtt ON CACHE  BOOL "Set mqtt on" FORCE )
set(skip_samples ON CACHE  BOOL "Set slip_samples on" FORCE )
set(BUILD_TESTING OFF CACHE  BOOL "Set BUILD_TESTING off" FORCE )

# Add Azure IoT SDK C
add_subdirectory(azure-iot-sdk-c out)

compileAsC99()

set(iothub_project_files
    your_application_file.c
)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(iothub_c_files ${iothub_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)

add_executable(iothub_device_sample ${iothub_project_files})

target_link_libraries(iothub_device_sample iothub_client)
```
