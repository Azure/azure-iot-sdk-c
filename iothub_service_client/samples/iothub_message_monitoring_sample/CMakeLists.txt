#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_message_monitoring_sample

compileAsC99()

# if(NOT ${use_http})
# 	message(FATAL_ERROR "iothub_message_monitoring_sample being generated without HTTP support")
# endif()

set(iothub_message_monitoring_sample_c_files
iothub_message_monitoring_sample.c
)

set(iothub_message_monitoring_sample_h_files
)

IF(WIN32)
	#windows needs this define
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(.)

add_executable(iothub_message_monitoring_sample ${iothub_message_monitoring_sample_c_files} ${iothub_message_monitoring_sample_h_files})

target_link_libraries(iothub_message_monitoring_sample 
	iothub_service_client
)

linkSharedUtil(iothub_message_monitoring_sample)
linkUAMQP(iothub_message_monitoring_sample)
