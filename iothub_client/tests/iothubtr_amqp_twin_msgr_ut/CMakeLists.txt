#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

compileAsC99()
set(theseTestsName iothubtr_amqp_twin_msgr_ut )

if(WIN32)
    if (ARCHITECTURE STREQUAL "x86_64")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
	endif()
endif()

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
	../../src/iothubtransport_amqp_twin_messenger.c
	${SHARED_UTIL_REAL_TEST_FOLDER}/real_singlylinkedlist.c
	${SHARED_UTIL_REAL_TEST_FOLDER}/real_constbuffer.c
)

set(${theseTestsName}_h_files
)

file(COPY ../global_valgrind_suppression.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

build_c_test_artifacts(${theseTestsName} ON "tests/azure_iothub_client_tests"
  ADDITIONAL_LIBS
      aziotsharedutil
  VALGRIND_SUPPRESSIONS_FILE
      global_valgrind_suppression.supp
)
