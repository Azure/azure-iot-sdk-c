#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

if(NOT (${use_amqp}))
   message(FATAL_ERROR "iothubclient_amqp_ws_e2e_sfc being generated without AMQP support")
endif()

compileAsC11()
set(theseTestsName iothubclient_amqp_ws_e2e_sfc)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
    ../common_e2e/iothubclient_common_e2e.c
)

set(${theseTestsName}_nuget_test_files
    ${theseTestsName}.c
    ../common_e2e/iothubclient_common_e2e.c
    ../../../certs/certs.c
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
                   iothub_client_amqp_ws_transport
                   iothub_service_client
                   aziotsharedutil
                   rpcrt4
           )
           linkSharedUtil(${theseTestsName}_dll)
           linkUAMQP(${theseTestsName}_dll)
   endif()

   if(TARGET ${theseTestsName}_exe)
           target_link_libraries(${theseTestsName}_exe
                   iothub_test
                   iothub_client
                   iothub_client_amqp_ws_transport
                   iothub_service_client
                   aziotsharedutil
                   rpcrt4
           )
           linkSharedUtil(${theseTestsName}_exe)
           linkUAMQP(${theseTestsName}_exe)
   endif()

    if(TARGET ${theseTestsName}_nuget_exe)
           target_link_libraries(${theseTestsName}_nuget_exe
                   iothub_test
                   iothub_service_client
                   rpcrt4
           )
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
                   iothub_client_amqp_ws_transport
                   iothub_service_client
                   aziotsharedutil
           )
           target_link_libraries(${theseTestsName}_exe)
           linkSharedUtil(${theseTestsName}_exe)
           linkUAMQP(${theseTestsName}_exe)
   endif()
endif()

