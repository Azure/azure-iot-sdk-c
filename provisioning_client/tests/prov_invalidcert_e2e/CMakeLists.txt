#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName prov_invalidcert_e2e)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../../certs/certs.c
)

set(${theseTestsName}_h_files
)

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
endif()

file(COPY ../common_prov_e2e/prov_valgrind_suppression.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/azure_prov_e2e_tests" VALGRIND_SUPPRESSIONS_FILE prov_valgrind_suppression.supp)

if(WIN32)
    if(TARGET ${theseTestsName}_dll)
        target_link_libraries(${theseTestsName}_dll
                iothub_client
                prov_device_ll_client
                aziotsharedutil
        )
    endif()
    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
                iothub_client
                prov_device_ll_client
                aziotsharedutil
        )
    endif()

    if(TARGET ${theseTestsName}_dll)
        if(${use_mqtt})
            add_definitions(-DUSE_MQTT)
            target_link_libraries(${theseTestsName}_dll
                prov_mqtt_transport 
                prov_mqtt_ws_transport
            )
            linkMqttLibrary(${theseTestsName}_dll)
        endif()
        if(${use_amqp})
            add_definitions(-DUSE_AMQP)
            target_link_libraries(${theseTestsName}_dll
                prov_amqp_transport
                prov_amqp_ws_transport
            )
            linkUAMQP(${theseTestsName}_dll)
        endif()
        if(${use_http})
            add_definitions(-DUSE_HTTP)
            target_link_libraries(${theseTestsName}_dll
                prov_http_transport
                rpcrt4
            )
        endif()
    endif()
    if(TARGET ${theseTestsName}_exe)
        if(${use_mqtt})
            add_definitions(-DUSE_MQTT)
            target_link_libraries(${theseTestsName}_exe
                prov_mqtt_transport 
                prov_mqtt_ws_transport
            )
            linkMqttLibrary(${theseTestsName}_exe)
        endif()
        if(${use_amqp})
            add_definitions(-DUSE_AMQP)
            target_link_libraries(${theseTestsName}_exe
                prov_amqp_transport
                prov_amqp_ws_transport
            )
            linkUAMQP(${theseTestsName}_exe)
        endif()
        if(${use_http})
            add_definitions(-DUSE_HTTP)
            target_link_libraries(${theseTestsName}_exe
                prov_http_transport
                rpcrt4
            )
        endif()
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
            iothub_client
            prov_device_ll_client
            aziotsharedutil
        )
        if(${use_mqtt})
            add_definitions(-DUSE_MQTT)
            target_link_libraries(${theseTestsName}_exe
                prov_mqtt_transport 
                prov_mqtt_ws_transport
            )
            linkMqttLibrary(${theseTestsName}_exe)
        endif()
        if(${use_amqp})
            add_definitions(-DUSE_AMQP)
            target_link_libraries(${theseTestsName}_exe
                prov_amqp_transport
                prov_amqp_ws_transport
            )
            linkUAMQP(${theseTestsName}_exe)
        endif()
        if(${use_http})
            add_definitions(-DUSE_HTTP)
            target_link_libraries(${theseTestsName}_exe
                prov_http_transport
            )
        endif()
    endif()
endif()
