#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_x509_sample
cmake_minimum_required(VERSION 2.8.11)

if(WIN32)
    message(FATAL_ERROR "This CMake file is only support Linux builds!")
endif()

set(AZUREIOT_INC_FOLDER ".." "/usr/include/azureiot" "/usr/include/azureiot/inc")

include_directories(${AZUREIOT_INC_FOLDER})

set(iothub_client_x509_sample_c_files
    ../iothub_ll_client_x509_sample.c
)

add_definitions(-DUSE_HTTP)
add_definitions(-DUSE_AMQP)
add_definitions(-DUSE_MQTT)
add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)

include_directories("../../../../certs")
set(iothub_client_x509_sample_c_files ${iothub_client_x509_sample_c_files} ../../../../certs/certs.c)

add_executable(iothub_client_x509_sample ${iothub_client_x509_sample_c_files})

target_link_libraries(iothub_client_x509_sample
    iothub_client_mqtt_transport
    iothub_client_amqp_transport
    iothub_client_http_transport
    iothub_client
    umqtt
    prov_auth_client
    hsm_security_client
    uhttp
    aziotsharedutil
    parson
    uuid
    pthread
    curl
    ssl
    crypto
    m
)
