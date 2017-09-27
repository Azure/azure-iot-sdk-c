#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_device_twin
cmake_minimum_required(VERSION 2.8.11)

if(WIN32)
    message(FATAL_ERROR "This CMake file is only support Linux builds!")
endif()

set(AZUREIOT_INC_FOLDER ".." "/usr/include/azureiot" "/usr/include/azureiot/inc")

include_directories(${AZUREIOT_INC_FOLDER})

set(iothub_client_sample_device_twin_c_files
    ../iothub_client_sample_device_twin.c
)

set(iothub_client_sample_device_twin_h_files
)

add_executable(iothub_client_sample_device_twin ${iothub_client_sample_device_twin_c_files} ${iothub_client_sample_device_twin_h_files})

if(${use_amqp}) 
	target_link_libraries(iothub_client_sample_device_twin  
		iothub_client_amqp_transport
		uamqp
	)
endif()

if(${use_mqtt}) 
	target_link_libraries(iothub_client_sample_device_twin  
		iothub_client_mqtt_transport
		umqtt
	)
endif()

target_link_libraries(iothub_client_sample_device_twin  
	aziotsharedutil
    iothub_client 
    pthread
    curl
    ssl
    crypto
    m
)