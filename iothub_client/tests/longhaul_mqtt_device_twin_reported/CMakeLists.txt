#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for longhaul_mqtt_device_twin_reported

compileAsC99()

set(PROJECT_NAME "longhaul_mqtt_device_twin_reported")

if(NOT ${use_mqtt})
    message(FATAL_ERROR "longhaul_mqtt_device_twin_reported being generated without umqtt support")
endif()

set(project_c_files
    ${PROJECT_NAME}.c
)

set(project_h_files
    ${PROJECT_NAME}.h
)

includeMqtt()
build_c_test_longhaul_test(${PROJECT_NAME} ${project_c_files} ${project_h_files})

linkSharedUtil(${PROJECT_NAME})
linkMqttLibrary(${PROJECT_NAME})
linkUAMQP(${PROJECT_NAME})