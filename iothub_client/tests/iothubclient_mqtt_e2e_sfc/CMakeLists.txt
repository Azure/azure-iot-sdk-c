#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)

if(NOT (${use_amqp} AND ${use_mqtt}))
   message(FATAL_ERROR "iothubclient_mqtt_e2e_sfc being generated without AMQP and MQTT support")
endif()

compileAsC99()
set(theseTestsName iothubclient_mqtt_e2e_sfc)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
    ../common_e2e/iothubclient_common_e2e.c
)

set(${theseTestsName}_c_files
    ../../../certs/certs.c
)

set(${theseTestsName}_h_files
    ../common_e2e/iothubclient_common_e2e.h
)

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
endif()

include_directories(../common_e2e)
include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${IOTHUB_SERVICE_CLIENT_INC_FOLDER})

file(COPY ../global_valgrind_suppression.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/E2ETests" VALGRIND_SUPPRESSIONS_FILE global_valgrind_suppression.supp)

if(WIN32)
   if(TARGET ${theseTestsName}_dll)
           target_link_libraries(${theseTestsName}_dll
                   iothub_test
                   iothub_client
                   iothub_client_mqtt_transport
                   iothub_service_client
                   aziotsharedutil
                   rpcrt4
           )
           linkUAMQP(${theseTestsName}_dll)
           linkMqttLibrary(${theseTestsName}_dll)
   endif()

   if(TARGET ${theseTestsName}_exe)
           target_link_libraries(${theseTestsName}_exe
                   iothub_test
                   iothub_client
                   iothub_client_mqtt_transport
                   iothub_service_client
                   aziotsharedutil
                   rpcrt4
           )
           linkUAMQP(${theseTestsName}_exe)
           linkMqttLibrary(${theseTestsName}_exe)
   endif()
else()
    if(UNIX) #LINUX OR APPLE
        find_package(PkgConfig REQUIRED)
        pkg_search_module(UUID REQUIRED uuid)
        link_directories(${UUID_LIBRARY_DIRS})
    endif()

    if(APPLE)
        target_link_libraries(${theseTestsName}_exe -L${UUID_LIBRARY_DIRS} pthread ${UUID_LIBRARIES})
    elseif(LINUX)
        target_link_libraries(${theseTestsName}_exe pthread ${UUID_LIBRARIES})
    endif()

   if(TARGET ${theseTestsName}_exe)
           target_link_libraries(${theseTestsName}_exe
                   iothub_test
                   iothub_client
                   iothub_client_mqtt_transport
                   iothub_service_client
                   aziotsharedutil
                   iothub_client_mqtt_transport
           )
           linkUAMQP(${theseTestsName}_exe)
           linkMqttLibrary(${theseTestsName}_exe)
   endif()
endif()
