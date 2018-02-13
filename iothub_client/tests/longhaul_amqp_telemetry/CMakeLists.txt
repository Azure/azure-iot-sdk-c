#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for longhaul_amqp_telemetry

compileAsC99()

if(NOT ${use_amqp})
	message(FATAL_ERROR "longhaul_amqp_telemetry being generated without uamqp support")
endif()

set(longhaul_amqp_telemetry_c_files
	../common_longhaul/iothub_client_statistics.c
	../common_longhaul/iothub_client_common_longhaul.c
	longhaul_amqp_telemetry.c
)

set(longhaul_amqp_telemetry_h_files
	../common_longhaul/iothub_client_statistics.h
	../common_longhaul/iothub_client_common_longhaul.h
	longhaul_amqp_telemetry.h
)

IF(WIN32)
	#windows needs this define
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${set_trusted_cert_in_samples})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
	include_directories(${PROJECT_SOURCE_DIR}/certs)
	set(samples_cert_file ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${IOTHUB_SERVICE_CLIENT_INC_FOLDER})
include_directories(.)

add_executable(longhaul_amqp_telemetry ${longhaul_amqp_telemetry_c_files} ${longhaul_amqp_telemetry_h_files} ${samples_cert_file})

target_link_libraries(longhaul_amqp_telemetry
		iothub_test
		iothub_client
		iothub_client_amqp_transport)

set_target_properties(longhaul_amqp_telemetry
                   PROPERTIES
                   FOLDER "tests/LongHaul")
				   
linkSharedUtil(longhaul_amqp_telemetry)
linkUAMQP(longhaul_amqp_telemetry)
