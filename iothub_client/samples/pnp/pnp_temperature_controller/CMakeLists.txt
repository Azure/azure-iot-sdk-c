#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt})
    message(FATAL_ERROR "pnp_temperatur_controller being generated without mqtt support")
endif()

compileAsC99()

set(PNP_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)

# Sample executable
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

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)

    set(iothub_client_device_twin_and_methods_sample_c_files
        ${iothub_client_device_twin_and_methods_sample_c_files}
        ${PROJECT_SOURCE_DIR}/certs/certs.c
    )
    include_directories(${PROJECT_SOURCE_DIR}/certs)
endif()

add_executable(pnp_temperature_controller
    ${pnp_temperature_controller_c_files}
)

include_directories(${PNP_COMMON_DIR})

target_link_libraries(pnp_temperature_controller
    aziotsharedutil
    iothub_client
    iothub_client_mqtt_transport
    iothub_client_mqtt_ws_transport
)

if(${use_prov_client})
    target_link_libraries(pnp_temperature_controller
        prov_device_ll_client
        prov_mqtt_transport
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
target_link_libraries(pnp_temperature_controller parson)
