#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName iothub_invalidcert_e2e)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../../certs/certs.c
)

set(${theseTestsName}_h_files
)

include_directories(../common_e2e)
include_directories(${IOTHUB_TEST_INC_FOLDER})
include_directories(${IOTHUB_SERVICE_CLIENT_INC_FOLDER})

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(../../../certs/)
endif()

file(COPY ../global_valgrind_suppression.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/E2ETests" VALGRIND_SUPPRESSIONS_FILE global_valgrind_suppression.supp)

if(WIN32)
    if(TARGET ${theseTestsName}_dll)
        if(${use_mqtt})
            add_definitions(-DUSE_MQTT)
            target_link_libraries(${theseTestsName}_dll
                iothub_client_mqtt_transport
                iothub_client_mqtt_ws_transport
            )
            linkMqttLibrary(${theseTestsName}_dll)
        endif()
        if(${use_amqp})
            add_definitions(-DUSE_AMQP)
            target_link_libraries(${theseTestsName}_dll
                iothub_client_amqp_transport
                iothub_client_amqp_ws_transport
            )
            linkUAMQP(${theseTestsName}_dll)
        endif()
        if(${use_http})
            add_definitions(-DUSE_HTTP)
            target_link_libraries(${theseTestsName}_dll
                iothub_client_http_transport
                rpcrt4
            )
        endif()
    endif()
    if(TARGET ${theseTestsName}_exe)
        if(${use_mqtt})
            add_definitions(-DUSE_MQTT)
            target_link_libraries(${theseTestsName}_exe
                iothub_client_mqtt_transport
                iothub_client_mqtt_ws_transport
            )
            linkMqttLibrary(${theseTestsName}_exe)
        endif()
        if(${use_amqp})
            add_definitions(-DUSE_AMQP)
            target_link_libraries(${theseTestsName}_exe
                iothub_client_amqp_transport
                iothub_client_amqp_ws_transport
            )
            linkUAMQP(${theseTestsName}_exe)
        endif()
        if(${use_http})
            add_definitions(-DUSE_HTTP)
            target_link_libraries(${theseTestsName}_exe
                iothub_client_http_transport
                rpcrt4
            )
        endif()
    endif()

    if(TARGET ${theseTestsName}_dll)
        target_link_libraries(${theseTestsName}_dll
                iothub_client
                aziotsharedutil
        )
    endif()
    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
                iothub_client
                aziotsharedutil
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
        if(${use_mqtt})
            add_definitions(-DUSE_MQTT)
            target_link_libraries(${theseTestsName}_exe
                iothub_client_mqtt_transport
                iothub_client_mqtt_ws_transport
            )
            linkMqttLibrary(${theseTestsName}_exe)
        endif()
        if(${use_amqp})
            add_definitions(-DUSE_AMQP)
            target_link_libraries(${theseTestsName}_exe
                iothub_client_amqp_transport
                iothub_client_amqp_ws_transport
            )
            linkUAMQP(${theseTestsName}_exe)
        endif()
        if(${use_http})
            add_definitions(-DUSE_HTTP)
            target_link_libraries(${theseTestsName}_exe
                iothub_client_http_transport
            )
        endif()

        target_link_libraries(${theseTestsName}_exe
            iothub_test
            iothub_client
            aziotsharedutil
        )

    endif()
endif()
