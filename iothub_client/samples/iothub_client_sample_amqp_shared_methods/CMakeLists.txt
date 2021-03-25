#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_amqp})
    message(FATAL_ERROR "iothub_client_sample_amqp_shared_methods being generated without AMQP support")
endif()

compileAsC99()

# Sample Executable
set(iothub_client_sample_amqp_shared_methods_c_files
    iothub_client_sample_amqp_shared_methods.c
)

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)

    set(iothub_client_sample_amqp_shared_methods_c_files
        ${iothub_client_sample_amqp_shared_methods_c_files}
        ${PROJECT_SOURCE_DIR}/certs/certs.c
    )
    include_directories(${PROJECT_SOURCE_DIR}/certs)
endif()

add_executable(iothub_client_sample_amqp_shared_methods
    ${iothub_client_sample_amqp_shared_methods_c_files}
)

target_link_libraries(iothub_client_sample_amqp_shared_methods
    aziotsharedutil
    iothub_client
)

# AMQP Library
target_link_libraries(iothub_client_sample_amqp_shared_methods
    iothub_client_amqp_transport
    iothub_client_amqp_ws_transport
)
