#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt} AND NOT ${use_amqp} AND NOT ${use_http})
    message(FATAL_ERROR "iothub_client_device_twin_and_methods_sample being generated without protocol support")
endif()

compileAsC99()

# Sample executable
set(iothub_client_device_twin_and_methods_sample_c_files
    iothub_client_device_twin_and_methods_sample.c
)

add_executable(iothub_client_device_twin_and_methods_sample
    ${iothub_client_device_twin_and_methods_sample_c_files}
)

target_link_libraries(iothub_client_device_twin_and_methods_sample
    iothub_client
)

# Certs Library
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    target_link_libraries(iothub_client_device_twin_and_methods_sample certs)
endif()

# Parson Library
include_directories(${PROJECT_SOURCE_DIR}/deps/parson)
target_link_libraries(iothub_client_device_twin_and_methods_sample parson)
