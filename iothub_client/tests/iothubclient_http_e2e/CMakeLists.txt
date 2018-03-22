#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

if(NOT ${use_http})
   message(FATAL_ERROR "iothubclient_http_e2e being generated without HTTP support")
endif()

compileAsC11()
set(theseTestsName iothubclient_http_e2e)

set(${theseTestsName}_test_files
   ${theseTestsName}.c
)

set(${theseTestsName}_nuget_test_files
   ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
)

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
                   iothub_client_http_transport
                   iothub_service_client
                   aziotsharedutil
                   iothub_client_http_transport
                   rpcrt4
           )
           linkUAMQP(${theseTestsName}_dll)
           linkHttp(${theseTestsName}_dll)
   endif()

   if(TARGET ${theseTestsName}_exe)
           target_link_libraries(${theseTestsName}_exe
                   iothub_test
                   iothub_client
                   iothub_client_http_transport
                   iothub_service_client
                   aziotsharedutil
                   iothub_client_http_transport
                   rpcrt4
           )
           linkUAMQP(${theseTestsName}_exe)
           linkHttp(${theseTestsName}_exe)
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
    endif()

    if(APPLE)
        find_package(PkgConfig REQUIRED)
        pkg_search_module(UUID REQUIRED uuid)
        link_directories(${UUID_LIBRARY_DIRS})
        target_link_libraries(${theseTestsName}_exe -L${UUID_LIBRARY_DIRS} pthread ${UUID_LIBRARIES})
    elseif(LINUX)
        target_link_libraries(${theseTestsName}_exe pthread uuid)
    endif()

    if(TARGET ${theseTestsName}_exe)
           target_link_libraries(${theseTestsName}_exe
                   iothub_test
                   iothub_client
                   iothub_client_http_transport
                   iothub_service_client
                   aziotsharedutil
                   iothub_client_http_transport
           )
           linkUAMQP(${theseTestsName}_exe)
           linkHttp(${theseTestsName}_exe)
   endif()
endif()

