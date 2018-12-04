#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_ll_c2d_sample
cmake_minimum_required(VERSION 2.8.11)

if(WIN32)
    message(FATAL_ERROR "This CMake file is only support Linux builds!")
endif()

set(AZUREIOT_INC_FOLDER ".." "/usr/include/azureiot" "/usr/include/azureiot/inc")

include_directories(${AZUREIOT_INC_FOLDER})

set(iothub_c_files
    ../iothub_ll_c2d_sample.c
    ../../../../certs/certs.c
)

add_definitions(-DUSE_HTTP)
add_definitions(-DUSE_AMQP)
add_definitions(-DUSE_MQTT)

add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
include_directories("../../../../certs")

add_executable(iothub_ll_c2d_sample ${iothub_c_files})

target_link_libraries(iothub_ll_c2d_sample
    iothub_client_mqtt_transport
    iothub_client_amqp_transport
    iothub_client_http_transport
    iothub_client
    umqtt
    prov_auth_client
    hsm_security_client
    uhttp
    aziotsharedutil
    uuid
    parson
    pthread
    curl
    ssl
    crypto
    m
)
