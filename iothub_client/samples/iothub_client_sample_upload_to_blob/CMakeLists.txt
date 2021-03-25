#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_http})
    message(FATAL_ERROR "iothub_client_sample_upload_to_blob being generated without HTTP support")
endif()

compileAsC99()

# Sample executable
set(iothub_client_sample_upload_to_blob_c_files
    iothub_client_sample_upload_to_blob.c
)

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)

    set(iothub_client_device_twin_and_methods_sample_c_files
        ${iothub_client_device_twin_and_methods_sample_c_files}
        ${PROJECT_SOURCE_DIR}/certs/certs.c
    )
    include_directories(${PROJECT_SOURCE_DIR}/certs)
endif()

add_executable(iothub_client_sample_upload_to_blob
    ${iothub_client_sample_upload_to_blob_c_files}
)

target_link_libraries(iothub_client_sample_upload_to_blob
    aziotsharedutil
    iothub_client
    iothub_client_http_transport
)
