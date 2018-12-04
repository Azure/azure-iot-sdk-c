#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_upload_to_blob
cmake_minimum_required(VERSION 2.8.11)

if(WIN32)
    message(FATAL_ERROR "This CMake file is only support Linux builds!")
endif()

set(AZUREIOT_INC_FOLDER ".." "/usr/include/azureiot" "/usr/include/azureiot/inc")

include_directories(${AZUREIOT_INC_FOLDER})

set(iothub_client_sample_upload_to_blob_c_files
    ../iothub_client_sample_upload_to_blob.c
)

set(iothub_client_sample_upload_to_blob_h_files
)

add_executable(iothub_client_sample_upload_to_blob ${iothub_client_sample_upload_to_blob_c_files} ${iothub_client_sample_upload_to_blob_h_files})

target_link_libraries(iothub_client_sample_upload_to_blob
    iothub_client
    iothub_client_http_transport
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
