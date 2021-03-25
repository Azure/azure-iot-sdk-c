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

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)

    set(iothub_client_device_twin_and_methods_sample_c_files
        ${iothub_client_device_twin_and_methods_sample_c_files}
        ${PROJECT_SOURCE_DIR}/certs/certs.c
    )
    include_directories(${PROJECT_SOURCE_DIR}/certs)
endif()

add_executable(iothub_client_device_twin_and_methods_sample
    ${iothub_client_device_twin_and_methods_sample_c_files}
)

target_link_libraries(iothub_client_device_twin_and_methods_sample
    aziotsharedutil
    iothub_client
)

# MQTT Library
if(${use_mqtt})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_mqtt_transport
        iothub_client_mqtt_ws_transport
    )
endif()

# AMQP Library
if(${use_amqp})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_amqp_transport
        iothub_client_amqp_ws_transport
    )
endif()

# HTTP Library
if(${use_http})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_http_transport
    )
endif()

# Parson Library
if(TARGET NOT ${parson})
    add_library(parson
        ./deps/parson/parson.c
        ./deps/parson/parson.h
    )
endif()

if (MSVC)
    set_source_files_properties(../deps/parson/parson.c PROPERTIES COMPILE_FLAGS "/wd4244 /wd4232")
endif()

include_directories(${PROJECT_SOURCE_DIR}/deps/parson)
target_link_libraries(iothub_client_device_twin_and_methods_sample parson)
