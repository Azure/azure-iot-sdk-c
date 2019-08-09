#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for digitaltwin_sample_device

compileAsC99()

set(digitaltwin_sample_device_c_files
    ./digitaltwin_sample_device.c
)

include_directories(. ${SHARED_UTIL_INC_FOLDER} ${IOTHUB_CLIENT_INC_FOLDER} ${DIGITALTWIN_CLIENT_INC_FOLDER} ${DIGITALTWIN_SAMPLE_DEVICE_INFO_INC_FOLDER} ${DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INC_FOLDER} ${DIGITALTWIN_SAMPLE_MODEL_DEFINITION_INC_FOLDER})

add_executable(digitaltwin_sample_device ${digitaltwin_sample_device_c_files})
file(COPY ${DIGITALTWIN_SAMPLE_ENVIRONMENTAL_SENSOR_INC_FOLDER}/EnvironmentalSensor.interface.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(digitaltwin_sample_device parson iothub_client_mqtt_transport iothub_client_mqtt_ws_transport iothub_client digitaltwin_client digitaltwin_sample_device_info digitaltwin_sample_environmental_sensor digitaltwin_sample_model_definition)