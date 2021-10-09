#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

cmake_policy(SET CMP0074 NEW)

project(ll_client_sample_amqp_shared)

if(WIN32)
    message(FATAL_ERROR "This CMake file only supports Linux builds!")
endif()

find_package(azure_iot_sdks REQUIRED)

add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
include_directories("../../../../certs")

set(iothub_ll_client_sample_amqp_shared_c_files
    ../iothub_ll_client_sample_amqp_shared.c
    ../../../../certs/certs.c
)

set(iothub_ll_client_sample_amqp_shared_h_files
    ../iothub_ll_client_sample_amqp_shared.h
)

add_executable(iothub_ll_client_sample_amqp_shared ${iothub_ll_client_sample_amqp_shared_c_files} ${iothub_ll_client_sample_amqp_shared_h_files})

target_link_libraries(iothub_ll_client_sample_amqp_shared
    aziotsharedutil
    iothub_client
    iothub_client_amqp_transport
)
