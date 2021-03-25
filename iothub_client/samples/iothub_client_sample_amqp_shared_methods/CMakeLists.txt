#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_amqp})
    message(FATAL_ERROR "iothub_client_sample_amqp_shared being generated without uamqp support")
endif()

compileAsC99()

# Compiler Defintions
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
endif()

# Sample executable
set(iothub_client_sample_amqp_shared_methods_c_files
    iothub_client_sample_amqp_shared_methods.c
)
set(iothub_client_sample_amqp_shared_methods_h_files
)

if(${use_sample_trusted_cert})
    set(iothub_client_sample_amqp_shared_methods_c_files
        ${iothub_client_sample_amqp_shared_methods_c_files}
        ${PROJECT_SOURCE_DIR}/certs/certs.c
    )
endif()

add_executable(iothub_client_sample_amqp_shared_methods
    ${iothub_client_sample_amqp_shared_methods_c_files}
    ${iothub_client_sample_amqp_shared_methods_h_files}
)

include_directories(.)
if(${use_sample_trusted_cert})
    include_directories(${PROJECT_SOURCE_DIR}/certs)
endif()

target_link_libraries(iothub_client_sample_amqp_shared_methods
    aziotsharedutil
    iothub_client
    iothub_client_amqp_transport
    iothub_client_amqp_ws_transport
)
