cmake_minimum_required(VERSION 2.8.11)

set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")

set(AZUREIOT_INC_FOLDER ".." "/usr/local/include/azureiot" "/usr/local/include/azureiot/inc")

include_directories(${AZUREIOT_INC_FOLDER})

set(sample_application_c_files
    ../RemoteMonitoring.c
    ../parson.c
)

set(sample_application_h_files
    ../parson.h
)

add_executable(RemoteMonitoringClient ${sample_application_c_files} ${sample_application_h_files})

target_link_libraries(RemoteMonitoringClient
    iothub_client
    iothub_client_mqtt_transport
    iothub_client_http_transport
    aziotsharedutil
    uuid
    umqtt
    uhttp
    pthread
    curl
    ssl
    crypto
    m
)