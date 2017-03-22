#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothubclient_mqtt_device_method_e2e
cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName iothubclient_badnetwork_e2e)
set(COMMON_E2E_DIR ../../../iothub_client/tests/common_e2e)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
    ${COMMON_E2E_DIR}/iothubclient_common_e2e.c
    badnetwork.c
    network_disconnect.c
)


set(${theseTestsName}_nuget_c_files
    ${theseTestsName}.c
    ${COMMON_E2E_DIR}/iothubclient_common_e2e.c
     ../../../certs/certs.c
    badnetwork.c
    network_disconnect.c
)

set(${theseTestsName}_c_files
    ../../../certs/certs.c
)

set(${theseTestsName}_h_files
    ${COMMON_E2E_DIR}/iothubclient_common_e2e.h
    badnetwork.h
    network_disconnect.h
)

include_directories(${COMMON_E2E_DIR})

file(COPY suppressions.supp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
build_c_test_artifacts(${theseTestsName} ON "tests/E2ETests" VALGRIND_SUPPRESSIONS_FILE suppressions.supp)

if(WIN32)
    if(TARGET ${theseTestsName}_dll)
        target_link_libraries(${theseTestsName}_dll
            iothub_test
            iothub_client
            iothub_client_amqp_transport
            iothub_client_mqtt_transport
            iothub_client_http_transport
            iothub_client_amqp_ws_transport
            iothub_client_mqtt_ws_transport
            iothub_service_client
            aziotsharedutil
            rpcrt4          
        )
        linkSharedUtil(${theseTestsName}_dll)
        linkUAMQP(${theseTestsName}_dll)
        linkMqttLibrary(${theseTestsName}_dll)
        linkHttp(${theseTestsName}_dll)
    endif()
    
    if(TARGET ${theseTestsName}_exe)
        target_link_libraries(${theseTestsName}_exe
            iothub_test
            iothub_client
            iothub_client_amqp_transport
            iothub_client_mqtt_transport
            iothub_client_http_transport
            iothub_client_amqp_ws_transport
            iothub_client_mqtt_ws_transport
            iothub_service_client
            aziotsharedutil
            rpcrt4          
        )
        linkSharedUtil(${theseTestsName}_exe)
        linkUAMQP(${theseTestsName}_exe)    
        linkMqttLibrary(${theseTestsName}_exe)
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
            iothub_client_amqp_transport
            iothub_client_mqtt_transport
            iothub_client_http_transport
            iothub_client_amqp_ws_transport
            iothub_client_mqtt_ws_transport
            iothub_service_client
            aziotsharedutil
        )
        linkSharedUtil(${theseTestsName}_exe)
        linkUAMQP(${theseTestsName}_exe)    
        linkMqttLibrary(${theseTestsName}_exe)
    endif()
endif()

