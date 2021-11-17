#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.5)

cmake_policy(SET CMP0074 NEW)

if(WIN32)
    message(FATAL_ERROR "This CMake file only supports Linux builds!")
endif()

find_package(azure_iot_sdks REQUIRED)

add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
include_directories("../../../../certs")

set(iothub_c_files
    ../iothub_ll_telemetry_sample.c
    ../../../../certs/certs.c
)

add_executable(iothub_ll_telemetry_sample ${iothub_c_files})

target_link_libraries(iothub_ll_telemetry_sample
    iothub_client
)
