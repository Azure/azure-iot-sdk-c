#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt})
    message(FATAL_ERROR "iotedge_downstream_device_sample being generated without protocol support")
endif()

compileAsC99()

# Compiler Defintions
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
endif()

# Sample executable
set(iothub_client_sample_module_filter_c_files
    iothub_client_sample_module_filter.c
)
set(iothub_client_sample_module_filter_h_files
)

if(${use_sample_trusted_cert})
    set(iothub_client_sample_module_filter_c_files
        ${iothub_client_sample_module_filter_c_files}
        ${PROJECT_SOURCE_DIR}/certs/certs.c
    )
endif()

add_executable(iothub_client_sample_module_filter
    ${iothub_client_sample_module_filter_c_files}
    ${iothub_client_sample_module_filter_h_files}
)

include_directories(.)
if(${set_trusted_cert_in_samples})
	include_directories(${PROJECT_SOURCE_DIR}/certs)
endif()

target_link_libraries(iothub_client_sample_module_filter
    aziotsharedutil
    iothub_client
    iothub_client_mqtt_transport
)
