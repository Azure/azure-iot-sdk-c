#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_sample_http

compileAsC99()

if(NOT ${use_http})
	message(FATAL_ERROR "iothub_client_sample_http being generated without HTTP support")
endif()

set(iothub_client_sample_http_c_files
iothub_client_sample_http_shared.c
)

if(WIN32)
	set(iothub_client_sample_http_c_files ${iothub_client_sample_http_c_files} ./windows/main.c)
else()
	set(iothub_client_sample_http_c_files ${iothub_client_sample_http_c_files} ./linux/main.c)
endif()


set(iothub_client_sample_http_h_files
iothub_client_sample_http_shared.h
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

include_directories(.)

add_executable(iothub_client_sample_http_shared ${iothub_client_sample_http_c_files} ${iothub_client_sample_http_h_files} ${samples_cert_file})

target_link_libraries(iothub_client_sample_http_shared 
#iothubclient is here only because locking... in gballoc no less.
	iothub_client
	iothub_client_http_transport
)

linkSharedUtil(iothub_client_sample_http_shared)
linkHttp(iothub_client_sample_http_shared)