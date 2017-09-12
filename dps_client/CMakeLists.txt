#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

compileAsC99()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_DPS_MODULE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_DPS_MODULE")

add_subdirectory(./deps)

set(SECURITY_CLIENT_H_FILES 
    ./inc/azure_hub_modules/iothub_device_auth.h
    ./inc/azure_hub_modules/iothub_security_factory.h)

set(SECURITY_CLIENT_C_FILES 
    ./src/iothub_device_auth.c
    ./src/iothub_security_factory.c)

set(DPS_CLIENT_H_FILES 
    ./inc/azure_hub_modules/dps_sec_client.h
    ./inc/azure_hub_modules/secure_device_factory.h)

set(DPS_CLIENT_C_FILES 
    ./src/dps_sec_client.c
    ./src/secure_device_factory.c)

set(DPS_HSM_LIBRARY)

if (${dps_hsm_x509})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDPS_X509_TYPE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDPS_X509_TYPE")

    if (${dps_custom_hsm})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDPS_CUSTOM_HSM")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDPS_CUSTOM_HSM")

        set(SECURITY_CLIENT_H_FILES ${SECURITY_CLIENT_H_FILES} 
            ./adapters/custom_hsm_abstract.h)
        set(SECURITY_CLIENT_C_FILES ${SECURITY_CLIENT_C_FILES} 
            ./adapters/custom_hsm_abstract.c)

        set(DPS_CLIENT_H_FILES ${DPS_CLIENT_H_FILES}
            ./adapters/custom_hsm_abstract.h)
        set(DPS_CLIENT_C_FILES ${DPS_CLIENT_C_FILES}
            ./adapters/custom_hsm_abstract.c)

        set(DPS_HSM_LIBRARY ${CUSTOM_HSM_LIB})
    else()
        set(SECURITY_CLIENT_H_FILES ${SECURITY_CLIENT_H_FILES}
            ./adapters/iothub_security_x509.h)
        set(SECURITY_CLIENT_C_FILES ${SECURITY_CLIENT_C_FILES}
            ./adapters/iothub_security_x509.c)

        set(DPS_CLIENT_H_FILES ${DPS_CLIENT_H_FILES}
            ./adapters/dps_hsm_riot.h)
        set(DPS_CLIENT_C_FILES ${DPS_CLIENT_C_FILES}
            ./adapters/dps_hsm_riot.c)

        # Include the riot directories
        include_directories(./deps/RIoT/Emulator/DICE)
        include_directories(./deps/RIoT/Emulator/RIoT)
        include_directories(./deps/RIoT/Emulator/RIoT/RIoTCrypt/include)

        set(DPS_HSM_LIBRARY msr_riot)
    endif()
else()
    # Using Sas Token
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDPS_SAS_TOKEN_TYPE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDPS_SAS_TOKEN_TYPE")

    if (${dps_custom_hsm})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDPS_CUSTOM_HSM")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDPS_CUSTOM_HSM")

        set(SECURITY_CLIENT_H_FILES ${SECURITY_CLIENT_H_FILES} 
            ./adapters/custom_hsm_abstract.h)
        set(SECURITY_CLIENT_C_FILES ${SECURITY_CLIENT_C_FILES} 
            ./adapters/custom_hsm_abstract.c)

        set(DPS_CLIENT_H_FILES ${DPS_CLIENT_H_FILES}
            ./adapters/custom_hsm_abstract.h)
        set(DPS_CLIENT_C_FILES ${DPS_CLIENT_C_FILES}
            ./adapters/custom_hsm_abstract.c)

        set(DPS_HSM_LIBRARY ${CUSTOM_HSM_LIB})
    else()
        include_directories(${TPM_C_INC_FOLDER})

        set(SECURITY_CLIENT_H_FILES ${SECURITY_CLIENT_H_FILES} 
            ./adapters/iothub_security_sas.h)
        set(SECURITY_CLIENT_C_FILES ${SECURITY_CLIENT_C_FILES} 
            ./adapters/iothub_security_sas.c)

        set(DPS_CLIENT_H_FILES ${DPS_CLIENT_H_FILES}
            ./adapters/dps_hsm_tpm.h)
        set(DPS_CLIENT_C_FILES ${DPS_CLIENT_C_FILES}
            ./adapters/dps_hsm_tpm.c)

        if (${dps_hsm_tpm_simulator})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_EMULATOR_MODULE")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_EMULATOR_MODULE")
        endif()

        set(DPS_HSM_LIBRARY ${DPS_HSM_LIBRARY} utpm)
        if (WIN32)
            set(DPS_HSM_LIBRARY ${DPS_HSM_LIBRARY} Tbs)
        endif()
    endif()
endif()

function(linkDPS_device_auth whatExeIsBuilding)
    target_link_libraries(${whatExeIsBuilding} iothub_security_client)
    target_link_libraries(${whatExeIsBuilding} dps_security_client)
endfunction()

if(${use_openssl})
    add_definitions(-DUSE_OPENSSL)
endif()

set(DPS_SOURCE_C_FILES
    ./src/base32.c
    ./src/dps_client.c
    ../deps/parson/parson.c
    ./src/uhttp.c)

set(DPS_SOURCE_H_FILES
    ./inc/azure_hub_modules/base32.h
    ./inc/azure_hub_modules/dps_client.h
    ../deps/parson/parson.h
    ./inc/azure_hub_modules/uhttp.h)

if(MSVC)
    set_source_files_properties(../deps/parson/parson.c PROPERTIES COMPILE_FLAGS "/wd4244 /wd4232")
endif()

set(DEV_AUTH_MODULES_CLIENT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using iothub_client lib" FORCE)

include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/../deps/parson)
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/adapters)

if(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
  
    if(WINCE) # Be lax with WEC 2013 compiler
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
    ENDIF(WINCE)
ENDIF(WIN32)

add_library(dps_security_client ${DPS_CLIENT_C_FILES} ${DPS_CLIENT_H_FILES})
linkSharedUtil(dps_security_client)

target_link_libraries(dps_security_client ${DPS_HSM_LIBRARY})

# Iothub Security Client library
add_library(iothub_security_client ${SECURITY_CLIENT_C_FILES} ${SECURITY_CLIENT_H_FILES})
linkSharedUtil(iothub_security_client)

target_link_libraries(iothub_security_client ${DPS_HSM_LIBRARY} dps_security_client)

add_library(dps_client ${DPS_SOURCE_C_FILES} ${DPS_SOURCE_H_FILES})
linkSharedUtil(dps_client)
target_link_libraries(dps_client dps_security_client)

if (${use_http})
    set(DPS_HTTP_TRANSPORT_CLIENT_H_FILES 
        ./inc/azure_hub_modules/dps_transport.h
        ./inc/azure_hub_modules/dps_transport_http_client.h)
    set(DPS_HTTP_TRANSPORT_CLIENT_C_FILES 
        ./src/dps_transport_http_client.c)

    # DPS http Transport Client library
    add_library(dps_http_transport ${DPS_HTTP_TRANSPORT_CLIENT_C_FILES} ${DPS_HTTP_TRANSPORT_CLIENT_H_FILES})
    linkSharedUtil(dps_http_transport)

    target_link_libraries(dps_http_transport dps_client)
endif()

if (${use_amqp} AND ${DPS_PROTOCOL_FEATURE})
    include_directories(${UAMQP_INCLUDES} ${UAMQP_INC_FOLDER})

    set(DPS_AMQP_TRANSPORT_CLIENT_H_FILES 
        ./inc/azure_hub_modules/dps_transport.h
        ./inc/azure_hub_modules/dps_transport_amqp_client.h
        ./inc/azure_hub_modules/dps_transport_amqp_common.h
        ./inc/azure_hub_modules/dps_sasl_tpm.h)
    set(DPS_AMQP_TRANSPORT_CLIENT_C_FILES 
        ./src/dps_transport_amqp_client.c
        ./src/dps_transport_amqp_common.c
        ./src/dps_sasl_tpm.c)

    set(DPS_AMQP_WS_TRANSPORT_CLIENT_H_FILES 
        ./inc/azure_hub_modules/dps_transport.h
        ./inc/azure_hub_modules/dps_transport_amqp_ws_client.h
        ./inc/azure_hub_modules/dps_transport_amqp_common.h
        ./inc/azure_hub_modules/dps_sasl_tpm.h)
    set(DPS_AMQP_WS_TRANSPORT_CLIENT_C_FILES 
        ./src/dps_transport_amqp_ws_client.c
        ./src/dps_transport_amqp_common.c
        ./src/dps_sasl_tpm.c)

    add_library(dps_amqp_ws_transport ${DPS_AMQP_WS_TRANSPORT_CLIENT_C_FILES} ${DPS_AMQP_WS_TRANSPORT_CLIENT_H_FILES})
    linkSharedUtil(dps_amqp_ws_transport)
    target_link_libraries(dps_amqp_ws_transport uamqp)

    # DPS amqp Transport Client library
    add_library(dps_amqp_transport ${DPS_AMQP_TRANSPORT_CLIENT_C_FILES} ${DPS_AMQP_TRANSPORT_CLIENT_H_FILES})
    linkSharedUtil(dps_amqp_transport)
    target_link_libraries(dps_amqp_transport uamqp)
endif()

if (${use_mqtt} AND ${DPS_PROTOCOL_FEATURE})
    include_directories(${MQTT_INC_FOLDER})

    set(DPS_MQTT_TRANSPORT_CLIENT_H_FILES 
        ./inc/azure_hub_modules/dps_transport.h
        ./inc/azure_hub_modules/dps_transport_mqtt_client.h
        ./inc/azure_hub_modules/dps_transport_mqtt_common.h)
    set(DPS_MQTT_TRANSPORT_CLIENT_C_FILES 
        ./src/dps_transport_mqtt_client.c
        ./src/dps_transport_mqtt_common.c)

    set(DPS_MQTT_WS_TRANSPORT_CLIENT_H_FILES 
        ./inc/azure_hub_modules/dps_transport.h
        ./inc/azure_hub_modules/dps_transport_mqtt_ws_client.h
        ./inc/azure_hub_modules/dps_transport_mqtt_common.h)
    set(DPS_MQTT_WS_TRANSPORT_CLIENT_C_FILES 
        ./src/dps_transport_mqtt_ws_client.c
        ./src/dps_transport_mqtt_common.c)

    add_library(dps_mqtt_ws_transport ${DPS_MQTT_WS_TRANSPORT_CLIENT_C_FILES} ${DPS_MQTT_WS_TRANSPORT_CLIENT_H_FILES})
    linkSharedUtil(dps_mqtt_ws_transport)
    target_link_libraries(dps_mqtt_ws_transport umqtt)

    # DPS amqp Transport Client library
    add_library(dps_mqtt_transport ${DPS_MQTT_TRANSPORT_CLIENT_C_FILES} ${DPS_MQTT_TRANSPORT_CLIENT_H_FILES})
    linkSharedUtil(dps_mqtt_transport)
    target_link_libraries(dps_mqtt_transport umqtt)
endif()

if(NOT IN_OPENWRT)
    # Disable tests for OpenWRT
    add_subdirectory(tests)
endif()

add_subdirectory(tools)
