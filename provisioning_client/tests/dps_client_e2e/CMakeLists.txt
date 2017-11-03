#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName dps_client_e2e)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

if(WIN32)
    set(PROCESS_C_FILES
        platform_process_win32.c
    )
else()
    set(PROCESS_C_FILES
        platform_process_linux.c
    )
endif()

set(${theseTestsName}_c_files
    ../../../certs/certs.c
    dps_service_client.c
    ${PROCESS_C_FILES}
)

set(${theseTestsName}_h_files
    dps_service_client.h
    platform_process.h
)

include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})

file(COPY suppressions.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/DPS_e2e_tests" VALGRIND_SUPPRESSIONS_FILE suppressions.supp)

set(dps_transports)
if (${use_http})
    add_definitions(-DUSE_HTTP)
    set(dps_transports ${dps_transports} dps_http_transport)
endif()
if (${use_mqtt} AND ${DPS_PROTOCOL_FEATURE})
    add_definitions(-DUSE_MQTT)
    set(dps_transports ${dps_transports} dps_mqtt_transport dps_mqtt_ws_transport)
endif()
if (${use_amqp} AND ${DPS_PROTOCOL_FEATURE})
    add_definitions(-DUSE_AMQP)
    set(dps_transports ${dps_transports} dps_amqp_transport dps_amqp_ws_transport)
endif()

if(WIN32)
   if(TARGET ${theseTestsName}_dll)
           target_link_libraries(${theseTestsName}_dll
                iothub_test
                aziotsharedutil
                dps_client
                ${dps_transports}
                rpcrt4
           )
           linkSharedUtil(${theseTestsName}_dll)
   endif()

   if(TARGET ${theseTestsName}_exe)
           target_link_libraries(${theseTestsName}_exe
                iothub_test
                dps_client
                ${dps_transports}
                rpcrt4
           )
           linkSharedUtil(${theseTestsName}_exe)
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
                dps_client
                ${dps_transports}
                iothub_test
           )
           linkSharedUtil(${theseTestsName}_exe)
   endif()
endif()

