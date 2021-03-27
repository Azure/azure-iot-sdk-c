#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_amqp})
    message(FATAL_ERROR "iothub_ll_client_sample_amqp_shared being generated without AMQP support")
endif()

compileAsC99()

# Sample Executable
set(iothub_ll_client_sample_amqp_shared_c_files
    iothub_ll_client_sample_amqp_shared.c
)

add_executable(iothub_ll_client_sample_amqp_shared
    ${iothub_ll_client_sample_amqp_shared_c_files}
)

target_link_libraries(iothub_ll_client_sample_amqp_shared
    iothub_client
)

# Certs Library
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    target_link_libraries(iothub_ll_client_sample_amqp_shared certs)
endif()
