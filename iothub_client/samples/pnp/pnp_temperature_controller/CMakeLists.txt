#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt})
    message(FATAL_ERROR "pnp_temperatur_controller being generated without MQTT support")
endif()

compileAsC99()

set(PNP_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)

# Sample Executable
set(pnp_temperature_controller_c_files
    pnp_temperature_controller.c
    pnp_thermostat_component.c
    pnp_deviceinfo_component.c
    ${PNP_COMMON_DIR}/pnp_device_client_ll.c
    ${PNP_COMMON_DIR}/pnp_protocol.c
)

if(${use_prov_client})
    set(pnp_temperature_controller_c_files
        ${pnp_temperature_controller_c_files}
        ${PNP_COMMON_DIR}/pnp_dps_ll.c
    )
endif()

add_executable(pnp_temperature_controller
    ${pnp_temperature_controller_c_files}
)

include_directories(${PNP_COMMON_DIR})

target_link_libraries(pnp_temperature_controller
    aziotsharedutil
    iothub_client
)

# Certs Library
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    target_link_libraries(pnp_temperature_controller certs)
endif()

# MQTT Library
target_link_libraries(pnp_temperature_controller
    iothub_client_mqtt_transport
    iothub_client_mqtt_ws_transport
)

# Provisioning Client Library
if(${use_prov_client})
    target_link_libraries(pnp_temperature_controller
        prov_device_ll_client
        prov_mqtt_transport
    )
endif()

# Parson Library
include_directories(${PROJECT_SOURCE_DIR}/deps/parson)
target_link_libraries(pnp_temperature_controller parson)
