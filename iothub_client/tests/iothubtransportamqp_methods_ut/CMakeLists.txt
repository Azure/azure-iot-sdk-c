#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

if(NOT ${use_amqp})
	message(FATAL_ERROR "iothubtransportamqp_methods_ut being generated without AMQP support")
endif()

compileAsC99()
set(theseTestsName iothubtransportamqp_methods_ut)

if(WIN32)
    if (ARCHITECTURE STREQUAL "x86_64")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
	endif()
endif()

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
	../../src/iothubtransportamqp_methods.c
	${SHARED_UTIL_REAL_TEST_FOLDER}/real_crt_abstractions.c
)

set(${theseTestsName}_h_files
)

build_c_test_artifacts(${theseTestsName} ON "tests/azure_iothub_client_tests")
