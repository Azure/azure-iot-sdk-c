#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt} AND NOT ${use_amqp} AND NOT ${use_http})
    message(FATAL_ERROR "iothub_ll_client_x509_sample being generated without protocol support")
endif()

compileAsC99()

# Sample executable
set(iothub_ll_client_x509_sample_c_files
    iothub_ll_client_x509_sample.c
)

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)

    set(iothub_client_device_twin_and_methods_sample_c_files
        ${iothub_client_device_twin_and_methods_sample_c_files}
        ${PROJECT_SOURCE_DIR}/certs/certs.c
    )
    include_directories(${PROJECT_SOURCE_DIR}/certs)
endif()

add_executable(iothub_ll_client_x509_sample
    ${iothub_ll_client_x509_sample_c_files}
)

target_link_libraries(iothub_ll_client_x509_sample
    aziotsharedutil
    iothub_client
)

# MQTT Library
if(${use_mqtt})
    target_link_libraries(iothub_ll_client_x509_sample
        iothub_client_mqtt_transport
        iothub_client_mqtt_ws_transport
    )
endif()

# AMQP Library
if(${use_amqp})
    target_link_libraries(iothub_ll_client_x509_sample
        iothub_client_amqp_transport
        iothub_client_amqp_ws_transport
    )
endif()

# HTTP Library
if(${use_http})
    target_link_libraries(iothub_ll_client_x509_sample
        iothub_client_http_transport
    )
endif()
