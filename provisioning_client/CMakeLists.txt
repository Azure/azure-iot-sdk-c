#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC99()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_PROV_MODULE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_PROV_MODULE")

add_subdirectory(./deps)

set(AUTH_CLIENT_H_FILES
    ./inc/azure_prov_client/prov_auth_client.h
    ./inc/azure_prov_client/prov_security_factory.h
    ./inc/azure_prov_client/iothub_auth_client.h
    ./inc/azure_prov_client/iothub_security_factory.h)

set(AUTH_CLIENT_C_FILES
    ./src/prov_auth_client.c
    ./src/prov_security_factory.c
    ./src/iothub_auth_client.c
    ./src/iothub_security_factory.c)

set(HSM_CLIENT_LIBRARY)

if (${hsm_type_custom})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHSM_AUTH_TYPE_CUSTOM")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHSM_AUTH_TYPE_CUSTOM")

    set(HSM_CLIENT_H_FILES ${HSM_CLIENT_H_FILES} 
        ./adapters/custom_hsm_abstract.h)
    set(HSM_CLIENT_C_FILES ${HSM_CLIENT_H_FILES} 
        ./adapters/custom_hsm_abstract.c)

    set(HSM_CLIENT_LIBRARY ${CUSTOM_HSM_LIB})
endif()

if (${hsm_type_x509})
    # Using x509
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHSM_TYPE_X509")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHSM_TYPE_X509")

    set(HSM_CLIENT_H_FILES ${HSM_CLIENT_H_FILES} 
        ./adapters/hsm_client_riot.h
        ./adapters/hsm_client_x509_abstract.h)
    set(HSM_CLIENT_C_FILES ${HSM_CLIENT_C_FILES} 
        ./adapters/hsm_client_riot.c)

    # Include the riot directories
    include_directories(./deps/RIoT/Emulator/DICE)
    include_directories(./deps/RIoT/Emulator/RIoT)
    include_directories(./deps/RIoT/Emulator/RIoT/RIoTCrypt/include)

    set(HSM_CLIENT_LIBRARY ${HSM_CLIENT_LIBRARY} msr_riot)
endif()

if (${hsm_type_sastoken})
    # Using Sas Token
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHSM_TYPE_SAS_TOKEN")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHSM_TYPE_SAS_TOKEN")

    set(HSM_CLIENT_H_FILES ${HSM_CLIENT_H_FILES} 
        ./adapters/hsm_client_tpm.h
        ./adapters/hsm_client_tpm_abstract.h)
    set(HSM_CLIENT_C_FILES ${HSM_CLIENT_C_FILES} 
        ./adapters/hsm_client_tpm.c)

    if (${use_tpm_simulator})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_EMULATOR_MODULE")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_EMULATOR_MODULE")
    endif()

    include_directories(${TPM_C_INC_FOLDER})

    set(HSM_CLIENT_LIBRARY ${HSM_CLIENT_LIBRARY} utpm)
    if (WIN32)
        set(HSM_CLIENT_LIBRARY ${HSM_CLIENT_LIBRARY} Tbs)
    endif()
endif()

function(link_security_client whatExeIsBuilding)
    target_link_libraries(${whatExeIsBuilding} prov_auth_client)
    target_link_libraries(${whatExeIsBuilding} hsm_security_client)
endfunction()

if(${use_openssl})
    add_definitions(-DUSE_OPENSSL)
endif()

set(PROV_SOURCE_C_FILES
    ./src/prov_device_ll_client.c
    ../deps/parson/parson.c)

set(PROV_SOURCE_H_FILES
    ./inc/azure_prov_client/prov_device_ll_client.h
    ../deps/parson/parson.h)

if(MSVC)
    set_source_files_properties(../deps/parson/parson.c PROPERTIES COMPILE_FLAGS "/wd4244 /wd4232")
endif()

set(DEV_AUTH_MODULES_CLIENT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using iothub_client lib" FORCE)

include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/../deps/parson)
include_directories(${UHTTP_C_INC_FOLDER})
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/adapters)

if(${memory_trace})
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
endif()

if(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  
    if(WINCE) # Be lax with WEC 2013 compiler
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
    ENDIF(WINCE)
ENDIF(WIN32)

add_library(hsm_security_client ${HSM_CLIENT_C_FILES} ${HSM_CLIENT_H_FILES})
linkSharedUtil(hsm_security_client)
target_link_libraries(hsm_security_client ${HSM_CLIENT_LIBRARY})

add_library(prov_auth_client ${AUTH_CLIENT_C_FILES} ${AUTH_CLIENT_H_FILES})
linkSharedUtil(prov_auth_client)
target_link_libraries(prov_auth_client hsm_security_client)

add_library(prov_device_client ${PROV_SOURCE_C_FILES} ${PROV_SOURCE_H_FILES})
linkSharedUtil(prov_device_client)
link_security_client(prov_device_client)

if (${use_http})
    set(PROV_HTTP_CLIENT_H_FILES 
        ./inc/azure_prov_client/prov_transport.h
        ./inc/azure_prov_client/prov_transport_http_client.h)
    set(PROV_HTTP_CLIENT_C_FILES 
        ./src/prov_transport_http_client.c)

    # Provisioning http Transport Client library
    add_library(prov_http_transport ${PROV_HTTP_CLIENT_C_FILES} ${PROV_HTTP_CLIENT_H_FILES})
    linkSharedUtil(prov_http_transport)

    target_link_libraries(prov_http_transport prov_device_client uhttp)
endif()

if (${use_amqp})
    include_directories(${UAMQP_INCLUDES} ${UAMQP_INC_FOLDER})

    set(PROV_AMQP_CLIENT_H_FILES 
        ./inc/azure_prov_client/prov_transport.h
        ./inc/azure_prov_client/prov_transport_amqp_client.h
        ./inc/azure_prov_client/prov_transport_amqp_common.h
        ./inc/azure_prov_client/prov_sasl_tpm.h)
    set(PROV_AMQP_CLIENT_C_FILES 
        ./src/prov_transport_amqp_client.c
        ./src/prov_transport_amqp_common.c
        ./src/prov_sasl_tpm.c)

    set(PROV_AMQP_WS_CLIENT_H_FILES 
        ./inc/azure_prov_client/prov_transport.h
        ./inc/azure_prov_client/prov_transport_amqp_ws_client.h
        ./inc/azure_prov_client/prov_transport_amqp_common.h
        ./inc/azure_prov_client/prov_sasl_tpm.h)
    set(PROV_AMQP_WS_CLIENT_C_FILES 
        ./src/prov_transport_amqp_ws_client.c
        ./src/prov_transport_amqp_common.c
        ./src/prov_sasl_tpm.c)

    add_library(prov_amqp_ws_transport ${PROV_AMQP_WS_CLIENT_C_FILES} ${PROV_AMQP_WS_CLIENT_H_FILES})
    linkSharedUtil(prov_amqp_ws_transport)
    target_link_libraries(prov_amqp_ws_transport uamqp)

    # Provisioning amqp Transport Client library
    add_library(prov_amqp_transport ${PROV_AMQP_CLIENT_C_FILES} ${PROV_AMQP_CLIENT_H_FILES})
    linkSharedUtil(prov_amqp_transport)
    target_link_libraries(prov_amqp_transport uamqp)
endif()

if (${use_mqtt})
    include_directories(${MQTT_INC_FOLDER})

    set(PROV_MQTT_CLIENT_H_FILES 
        ./inc/azure_prov_client/prov_transport.h
        ./inc/azure_prov_client/prov_transport_mqtt_client.h
        ./inc/azure_prov_client/prov_transport_mqtt_common.h)
    set(PROV_MQTT_CLIENT_C_FILES 
        ./src/prov_transport_mqtt_client.c
        ./src/prov_transport_mqtt_common.c)

    set(PROV_MQTT_WS_CLIENT_H_FILES 
        ./inc/azure_prov_client/prov_transport.h
        ./inc/azure_prov_client/prov_transport_mqtt_ws_client.h
        ./inc/azure_prov_client/prov_transport_mqtt_common.h)
    set(PROV_MQTT_WS_CLIENT_C_FILES 
        ./src/prov_transport_mqtt_ws_client.c
        ./src/prov_transport_mqtt_common.c)

    add_library(prov_mqtt_ws_transport ${PROV_MQTT_WS_CLIENT_C_FILES} ${PROV_MQTT_WS_CLIENT_H_FILES})
    linkSharedUtil(prov_mqtt_ws_transport)
    target_link_libraries(prov_mqtt_ws_transport umqtt)

    # Provisioning mqtt Transport Client library
    add_library(prov_mqtt_transport ${PROV_MQTT_CLIENT_C_FILES} ${PROV_MQTT_CLIENT_H_FILES})
    linkSharedUtil(prov_mqtt_transport)
    target_link_libraries(prov_mqtt_transport umqtt)
endif()

if(NOT IN_OPENWRT)
    # Disable tests for OpenWRT
    add_subdirectory(tests)
endif()

add_subdirectory(tools)
