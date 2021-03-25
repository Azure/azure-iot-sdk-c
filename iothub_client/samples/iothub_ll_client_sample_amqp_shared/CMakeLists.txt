#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_amqp})
    message(FATAL_ERROR "iothub_ll_client_sample_amqp_shared being generated without uamqp support")
endif()

compileAsC99()

# Sample executable
set(iothub_ll_client_sample_amqp_shared_c_files
    iothub_ll_client_sample_amqp_shared.c
)

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)

    set(iothub_client_device_twin_and_methods_sample_c_files
        ${iothub_client_device_twin_and_methods_sample_c_files}
        ${PROJECT_SOURCE_DIR}/certs/certs.c
    )
    include_directories(${PROJECT_SOURCE_DIR}/certs)
endif()


add_executable(iothub_ll_client_sample_amqp_shared
    ${iothub_ll_client_sample_amqp_shared_c_files}
)

target_link_libraries(iothub_ll_client_sample_amqp_shared
    aziotsharedutil
    iothub_client
    iothub_client_amqp_transport
)
