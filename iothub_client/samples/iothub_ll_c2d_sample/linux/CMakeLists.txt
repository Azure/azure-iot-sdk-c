#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_ll_telemetry_sample
cmake_minimum_required(VERSION 3.5)

project(iothub_ll_telemetry_sample)

if(WIN32)
    message(FATAL_ERROR "This CMake file is only support Linux builds!")
endif()

find_package(azure_iot_sdks CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)

set(iothub_c_files
../../iothub_ll_telemetry_sample/iothub_ll_telemetry_sample.c
../../../../certs/certs.c
)

add_definitions(-DUSE_HTTP)
add_definitions(-DUSE_AMQP)
add_definitions(-DUSE_MQTT)

add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
include_directories("../../../../certs")

add_executable(iothub_ll_telemetry_sample ${iothub_c_files})

target_include_directories(iothub_ll_telemetry_sample PRIVATE ${CURL_INCLUDE_DIRS})
target_link_libraries(iothub_ll_telemetry_sample
    iothub_client_mqtt_transport
    iothub_client_amqp_transport
    iothub_client
    aziotsharedutil
    ${CURL_LIBRARIES}
    ZLIB::ZLIB
)
