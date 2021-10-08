#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

Option(edison "set edison to ON to build the DM edison sample" OFF)

cmake_minimum_required (VERSION 3.5)

project(iothub_client_sample_firmware_update)

macro(compileAsC99)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
    if (NOT IN_OPENWRT)
      set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()
    endif()
  else()
    set (CMAKE_C_STANDARD 99)
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(compileAsC99)

compileAsC99()

if(LINUX)

    set(AZUREIOT_INC_FOLDER "pi_device" "../../../serializer/inc" "." "/usr/include/azureiot" "/usr/include/azureiot/inc")

    include_directories(${AZUREIOT_INC_FOLDER})

    set(iothub_client_sample_mqtt_dm_c_files
        iothub_client_sample_mqtt_dm.c
        pi_device/pi.c
    )
    set(iothub_client_sample_mqtt_dm_h_files
        iothub_client_sample_mqtt_dm.h
    )

    add_executable(iothub_client_sample_mqtt_dm ${iothub_client_sample_mqtt_dm_c_files} ${iothub_client_sample_mqtt_dm_h_files})

    target_link_libraries(iothub_client_sample_mqtt_dm
        iothub_client
        iothub_client_mqtt_transport
        serializer
        aziotsharedutil
        umqtt
        pthread
        curl
        m
    )

endif()
