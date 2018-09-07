#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_upload_to_blob

compileAsC99()

if(NOT ${use_http})
    message(FATAL_ERROR "iothub_client_sample_upload_to_blob being generated without HTTP support")
endif()

set(iothub_client_sample_upload_to_blob_c_files
    iothub_client_sample_upload_to_blob.c
)

set(iothub_client_sample_upload_to_blob_h_files
)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(iothub_client_sample_upload_to_blob_c_files ${iothub_client_sample_upload_to_blob_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)

add_executable(iothub_client_sample_upload_to_blob ${iothub_client_sample_upload_to_blob_c_files} ${iothub_client_sample_upload_to_blob_h_files})

target_link_libraries(iothub_client_sample_upload_to_blob
    iothub_client
    iothub_client_http_transport
)

linkSharedUtil(iothub_client_sample_upload_to_blob)
linkHttp(iothub_client_sample_upload_to_blob)
