#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for devicetwin_simplesample

compileAsC99()

if(NOT ${use_mqtt} AND NOT ${use_amqp})
    message(FATAL_ERROR "devicetwin_simplesample being generated without mqtt and amqp support")
endif()

set(devicetwin_simplesample_c_files
    devicetwin_simplesample.c
)

set(devicetwin_simplesample_h_files
    devicetwin_simplesample.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(. ${SERIALIZER_INC_FOLDER} ${SHARED_UTIL_INC_FOLDER} ${IOTHUB_CLIENT_INC_FOLDER})

add_executable(devicetwin_simplesample ${devicetwin_simplesample_c_files} ${devicetwin_simplesample_h_files})

target_link_libraries(devicetwin_simplesample serializer iothub_client)

if(${use_amqp})
    target_link_libraries(devicetwin_simplesample iothub_client_amqp_transport)
    linkUAMQP(devicetwin_simplesample)
endif()

if(${use_mqtt})
    target_link_libraries(devicetwin_simplesample iothub_client_mqtt_transport)
endif()

linkSharedUtil(devicetwin_simplesample)



