#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

if(NOT ${use_mqtt})
    message(FATAL_ERROR "iothubclient_mqtt_mod_dt_e2e being generated without MQTT support")
endif()

compileAsC11()
set(theseTestsName iothubclient_mqtt_mod_dt_e2e)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
    ../common_dt_e2e/iothubclient_common_dt_e2e.c
)

set(${theseTestsName}_c_files
    ${theseTestsName}.c
    ../common_dt_e2e/iothubclient_common_dt_e2e.c
)

set(${theseTestsName}_nuget_test_files
    ${theseTestsName}.c
    ../common_dt_e2e/iothubclient_common_dt_e2e.c
)

set(${theseTestsName}_nuget_c_files
    ../../../certs/certs.c
)

set(${theseTestsName}_c_files
    ../../../certs/certs.c
    ../../../deps/parson/parson.c
)

set(${theseTestsName}_h_files
    ../../../certs/certs.h
    ../../../deps/parson/parson.h
    ../common_dt_e2e/iothubclient_common_dt_e2e.h
)

if(${use_wolfssl})
    add_definitions(-DUSE_WOLFSSL)
endif()

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(../../../certs)
endif()

include_directories(../common_dt_e2e)
include_directories(../../../deps/parson/)
include_directories(../../../certs/)
include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${IOTHUB_SERVICE_CLIENT_INC_FOLDER})

file(COPY ../global_valgrind_suppression.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/E2ETests" VALGRIND_SUPPRESSIONS_FILE global_valgrind_suppression.supp)

if(WIN32)
    if(MSVC)
        #CPP compiler on windows likes to complain about unused local function removed (C4505)
        #C compiler doesn't like to complain about the same thing
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4505")
        set_source_files_properties(../../../deps/parson/parson.c PROPERTIES COMPILE_FLAGS "/wd4244 /wd4232")
    endif()

    if(TARGET ${theseTestsName}_dll)
        target_link_libraries(${theseTestsName}_dll
            aziotsharedutil
            iothub_test
            iothub_client
            iothub_service_client
            iothub_client_mqtt_transport
            iothub_client_mqtt_ws_transport
            rpcrt4
        )
        linkMqttLibrary(${theseTestsName}_dll)
    endif()

    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            aziotsharedutil
            iothub_test
            iothub_client
            iothub_service_client
            iothub_client_mqtt_transport
            iothub_client_mqtt_ws_transport
            rpcrt4
        )
        linkMqttLibrary(${theseTestsName}_exe)
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
            aziotsharedutil
            iothub_test
            iothub_client
            iothub_client_mqtt_transport
            iothub_service_client
            aziotsharedutil
            iothub_client_mqtt_transport
            iothub_client_mqtt_ws_transport
        )
        linkMqttLibrary(${theseTestsName}_exe)
    endif()
endif()
