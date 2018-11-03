#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for iothub_client

if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

compileAsC99()

set(iothub_client_c_files
    ./src/iothub.c
    ./src/iothub_client.c
    ./src/iothub_client_core.c
    ./src/iothub_client_core_ll.c
    ./src/iothub_client_diagnostic.c
    ./src/iothub_client_ll.c
    ./src/iothub_device_client.c
    ./src/iothub_device_client_ll.c
    ./src/iothub_message.c
    ./src/iothub_module_client.c
    ./src/iothub_module_client_ll.c
    ./src/iothubtransport.c
    ./src/version.c
)

set(iothub_client_h_files
    ./inc/iothub.h
    ./inc/iothub_client_core.h
    ./inc/iothub_client_core_ll.h
    ./inc/iothub_client.h
    ./inc/iothub_client_core_common.h
    ./inc/iothub_client_ll.h
    ./inc/internal/iothub_client_diagnostic.h
    ./inc/iothub_client_options.h
    ./inc/internal/iothub_client_private.h
    ./inc/iothub_client_version.h
    ./inc/iothub_device_client.h
    ./inc/iothub_device_client_ll.h
    ./inc/iothub_module_client.h
    ./inc/iothub_module_client_ll.h
    ./inc/iothub_transport_ll.h
    ./inc/iothub_message.h
    ./inc/internal/iothubtransport.h
)

set(iothub_client_libs)

set(install_staticlibs
    iothub_client
)

if(NOT dont_use_uploadtoblob)
    set(iothub_client_c_files
        ${iothub_client_c_files}
        ./src/iothub_client_ll_uploadtoblob.c
        ./src/blob.c
    )

    set(iothub_client_h_files
        ${iothub_client_h_files}
        ./inc/internal/blob.h
        ./inc/internal/iothub_client_ll_uploadtoblob.h
    )
endif()

if (use_edge_modules)
    set(iothub_client_c_files
        ${iothub_client_c_files}
        ./src/iothub_client_edge.c
    )

    set (iothub_client_h_files
        ${iothub_client_h_files}
        ./inc/internal/iothub_client_edge.h
    )
endif()

#this is around for back compat only
if (${use_prov_client_core})
    set(iothub_client_h_files
        ${iothub_client_h_files}
        ./inc/internal/iothub_client_hsm_ll.h
    )
endif()
#end back compat only

set(iothub_client_h_install_files
    ${iothub_client_h_files}
)

if(${use_http})
    set(install_staticlibs ${install_staticlibs}
        iothub_client_http_transport
    )
    set(iothub_client_http_transport_c_files
        ./src/iothub_client_authorization.c
        ./src/iothub_client_retry_control.c
        ./src/iothub_transport_ll_private.c
        ./src/iothubtransporthttp.c
    )

    set(iothub_client_http_transport_h_files
        ./inc/internal/iothub_client_authorization.h
        ./inc/internal/iothub_client_retry_control.h
        ./inc/internal/iothub_transport_ll_private.h
        ./inc/iothubtransporthttp.h
        ./inc/iothub_transport_ll.h
    )

    set(iothub_client_h_install_files
        ${iothub_client_h_install_files}
        ${iothub_client_http_transport_h_files}
    )
endif()

if(${use_amqp})
    set(install_staticlibs ${install_staticlibs}
        iothub_client_amqp_transport
        iothub_client_amqp_ws_transport
    )

    set(iothub_client_amqp_transport_common_c_files
        ./src/iothub_client_authorization.c
        ./src/iothub_client_retry_control.c
        ./src/iothub_transport_ll_private.c
        ./src/iothubtransport_amqp_common.c
        ./src/iothubtransport_amqp_device.c
        ./src/iothubtransport_amqp_cbs_auth.c
        ./src/iothubtransport_amqp_connection.c
        ./src/iothubtransport_amqp_telemetry_messenger.c
        ./src/iothubtransport_amqp_twin_messenger.c
        ./src/iothubtransport_amqp_messenger.c
        ./src/iothubtransportamqp_methods.c
        ./src/message_queue.c
        ./src/uamqp_messaging.c
    )

    set(iothub_client_amqp_transport_common_h_files
        ./inc/internal/iothub_client_authorization.h
        ./inc/internal/iothub_client_retry_control.h
        ./inc/internal/iothub_transport_ll_private.h
        ./inc/internal/iothubtransport_amqp_common.h
        ./inc/internal/iothubtransport_amqp_device.h
        ./inc/internal/iothubtransport_amqp_cbs_auth.h
        ./inc/internal/iothubtransport_amqp_connection.h
        ./inc/internal/iothubtransport_amqp_telemetry_messenger.h
        ./inc/internal/iothubtransport_amqp_twin_messenger.h
        ./inc/internal/iothubtransport_amqp_messenger.h
        ./inc/internal/iothubtransportamqp_methods.h
        ./inc/internal/message_queue.h
        ./inc/internal/uamqp_messaging.h
    )

    set(iothub_client_amqp_transport_c_files
        ${iothub_client_amqp_transport_common_c_files}
        ./src/iothubtransportamqp.c
    )

    set(iothub_client_amqp_transport_h_files
        ${iothub_client_amqp_transport_common_h_files}
        ./inc/iothubtransportamqp.h
    )

    set(iothub_client_h_install_files
        ${iothub_client_h_install_files}
        ${iothub_client_amqp_transport_h_files}
    )

    set(iothub_client_amqp_ws_transport_c_files
        ${iothub_client_amqp_transport_common_c_files}
        ./src/iothubtransportamqp_websockets.c
    )

    set(iothub_client_amqp_ws_transport_h_files
        ${iothub_client_amqp_transport_common_h_files}
        ./inc/iothubtransportamqp_websockets.h
    )

    set(iothub_client_h_install_files
        ${iothub_client_h_install_files}
        ./inc/iothubtransportamqp_websockets.h
    )
endif()

if(${use_mqtt})
    set(install_staticlibs ${install_staticlibs}
        iothub_client_mqtt_transport
        iothub_client_mqtt_ws_transport
    )
    set(iothub_client_mqtt_ws_transport_c_files
        ./src/iothub_client_authorization.c
        ./src/iothub_client_retry_control.c
        ./src/iothub_transport_ll_private.c
        ./src/iothubtransport_mqtt_common.c
        ./src/iothubtransportmqtt_websockets.c
    )
    set(iothub_client_mqtt_ws_transport_h_files
        ./inc/internal/iothub_client_authorization.h
        ./inc/internal/iothub_client_retry_control.h
        ./inc/internal/iothub_transport_ll_private.h
        ./inc/internal/iothubtransport_mqtt_common.h
        ./inc/iothubtransportmqtt_websockets.h
    )

    set(iothub_client_mqtt_transport_c_files
        ./src/iothub_client_authorization.c
        ./src/iothub_client_retry_control.c
        ./src/iothub_transport_ll_private.c
        ./src/iothubtransport_mqtt_common.c
        ./src/iothubtransportmqtt.c
    )

    set(iothub_client_mqtt_transport_h_files
        ./inc/internal/iothub_client_authorization.h
        ./inc/internal/iothub_client_retry_control.h
        ./inc/internal/iothub_transport_ll_private.h
        ./inc/internal/iothubtransport_mqtt_common.h
        ./inc/iothubtransportmqtt.h
    )

    set(iothub_client_h_install_files
        ${iothub_client_h_install_files}
        ${iothub_client_mqtt_transport_h_files}
        ${iothub_client_mqtt_ws_transport_h_files}
    )
endif()

#these are the include folders
#the following "set" statetement exports across the project a global variable called SHARED_UTIL_INC_FOLDER that expands to whatever needs to included when using COMMON library

if(${use_http})
    set(IOTHUB_CLIENT_HTTP_TRANSPORT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using iothub_client_http_transport lib" FORCE)
endif()

if(${use_mqtt})
    set(IOTHUB_CLIENT_MQTT_TRANSPORT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using iothub_client_mqtt_transport lib" FORCE)
endif()

if(${use_amqp})
    set(IOTHUB_CLIENT_AMQP_TRANSPORT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using iothub_client_amqp_transport lib" FORCE)
endif()

set(IOTHUB_CLIENT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using iothub_client lib" FORCE)


if(NOT ${dont_use_uploadtoblob} OR ${use_edge_modules})
    include_directories(../deps/parson)
endif()

include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${AZURE_C_SHARED_UTILITY_INCLUDES})
include_directories(${SHARED_UTIL_INC_FOLDER})

if (WINCE)
    include_directories(${SHARED_UTIL_INC_FOLDER}/azure_c_shared_utility/windowsce) #windowsce SDK doesn't have stdbool.h
ENDIF()

include_directories(${IOTHUB_CLIENT_INC_FOLDER})

if(${memory_trace})
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
endif()

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    if (WINCE) # Be lax with WEC 2013 compiler
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
        SET_SOURCE_FILES_PROPERTIES(src/iothub_client.c src/iothubtransport.c src/iothub_client_ll.c src/iothubtransporthttp.c src/blob.c PROPERTIES LANGUAGE CXX)
    ENDIF(WINCE)
ENDIF(WIN32)

set(iothub_transport_source)

if(${use_http})
    include_directories(${IOTHUB_CLIENT_HTTP_TRANSPORT_INC_FOLDER})
    add_library(iothub_client_http_transport
        ${iothub_client_http_transport_c_files}
        ${iothub_client_http_transport_h_files}
    )
    setSdkTargetBuildProperties(iothub_client_http_transport)
    linkSharedUtil(iothub_client_http_transport)
    set(iothub_client_libs
        ${iothub_client_libs}
        iothub_client_http_transport
    )

    if(build_as_dynamic)
        set(iothub_transport_source ${iothub_transport_source}
            ${iothub_client_http_transport_c_files}
            ${iothub_client_http_transport_h_files})
    endif(build_as_dynamic)
endif()

if(${use_amqp})
    include_directories(${UAMQP_INCLUDES})
    include_directories(${IOTHUB_CLIENT_AMQP_TRANSPORT_INC_FOLDER} ${UAMQP_INC_FOLDER})
    add_library(iothub_client_amqp_transport
        ${iothub_client_amqp_transport_c_files}
        ${iothub_client_amqp_transport_h_files}
    )
    setSdkTargetBuildProperties(iothub_client_amqp_transport)
    linkSharedUtil(iothub_client_amqp_transport)
    set(iothub_client_libs
        ${iothub_client_libs}
        iothub_client_amqp_transport
    )

    include_directories(${iothub_client_amqp_ws_transport_INC_FOLDER} ${UAMQP_INC_FOLDER})
    add_library(iothub_client_amqp_ws_transport
        ${iothub_client_amqp_ws_transport_c_files}
        ${iothub_client_amqp_ws_transport_h_files}
    )
    setSdkTargetBuildProperties(iothub_client_amqp_ws_transport)
    linkSharedUtil(iothub_client_amqp_ws_transport)
    set(iothub_client_libs
        ${iothub_client_libs}
        iothub_client_amqp_ws_transport
    )

    if (build_as_dynamic)
        set(iothub_transport_source ${iothub_transport_source}
            ${iothub_client_amqp_transport_c_files}
            ${iothub_client_amqp_transport_h_files})

        set(iothub_transport_source ${iothub_transport_source}
            ${iothub_client_amqp_ws_transport_c_files}
            ${iothub_client_amqp_ws_transport_h_files})

        set(iothub_client_libs ${iothub_client_libs} uamqp)
    endif(build_as_dynamic)
endif()

if(${use_mqtt})
    include_directories(${UMQTT_INCLUDES})
    include_directories(${IOTHUB_CLIENT_MQTT_TRANSPORT_INC_FOLDER} ${MQTT_INC_FOLDER})
    add_library(iothub_client_mqtt_transport
        ${iothub_client_mqtt_transport_c_files}
        ${iothub_client_mqtt_transport_h_files}
    )
    setSdkTargetBuildProperties(iothub_client_mqtt_transport)
    linkSharedUtil(iothub_client_mqtt_transport)
    linkMqttLibrary(iothub_client_mqtt_transport)
    set(iothub_client_libs
        ${iothub_client_libs}
        iothub_client_mqtt_transport
    )
    include_directories(${IOTHUB_CLIENT_MQTT_TRANSPORT_INC_FOLDER} ${MQTT_INC_FOLDER})
    add_library(iothub_client_mqtt_ws_transport
        ${iothub_client_mqtt_ws_transport_c_files}
        ${iothub_client_mqtt_ws_transport_h_files}
    )
    setSdkTargetBuildProperties(iothub_client_mqtt_ws_transport)
    linkSharedUtil(iothub_client_mqtt_ws_transport)
    linkMqttLibrary(iothub_client_mqtt_ws_transport)
    set(iothub_client_libs
        ${iothub_client_libs}
        iothub_client_mqtt_ws_transport
    )

    if (build_as_dynamic)
        set(iothub_transport_source ${iothub_transport_source}
            ${iothub_client_mqtt_transport_c_files}
            ${iothub_client_mqtt_transport_h_files})

        set(iothub_transport_source ${iothub_transport_source}
            ${iothub_client_mqtt_ws_transport_c_files}
            ${iothub_client_mqtt_ws_transport_h_files})
    endif(build_as_dynamic)
endif()

include_directories(${IOTHUB_CLIENT_INC_FOLDER})

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

if (${build_as_dynamic})
    set(iothub_def_file ./src/iothub_client_dll.def)
    if(NOT ${dont_use_uploadtoblob})
        set(iothub_def_file ${iothub_def_file} ./src/upload_to_blob.def)
    endif()

    if(use_amqp)
        set(iothub_def_file ${iothub_def_file} ./src/iothub_transport_amqp.def)
    endif()

    if(use_http)
        set(iothub_def_file ${iothub_def_file} ./src/iothub_transport_http.def)
    endif()

    if(use_mqtt)
        set(iothub_def_file ${iothub_def_file} ./src/iothub_transport_mqtt.def)
    endif()

    if (${use_edge_modules})
        set(iothub_def_file ${iothub_def_file} ./src/iothub_edge_modules.def)
    endif()

    add_library(iothub_client_dll SHARED
        ${iothub_client_c_files}
        ${iothub_client_h_files}
        ${iothub_def_file}
        ${iothub_transport_source}
    )

    setSdkTargetBuildProperties(iothub_client_dll)
    target_link_libraries(iothub_client_dll ${iothub_client_libs})
    if(NOT WIN32)
        set_target_properties(iothub_client_dll PROPERTIES OUTPUT_NAME "iothub_client")
    endif()

    set_target_properties(iothub_client_dll PROPERTIES
        ARCHIVE_OUTPUT_NAME "iothub_client_dll_import"
        ENABLE_EXPORTS YES
        VERSION ${IOT_SDK_VERSION}
        SOVERSION ${IOT_SDK_VERION_MAJOR}
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    set(iothub_client_libs
        iothub_client_dll
        ${iothub_client_libs}
    )
    if (${use_prov_client_core})
        target_link_libraries(iothub_client_dll hsm_security_client prov_auth_client)
    endif()
    target_link_libraries(iothub_client_dll parson)

    if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
        target_link_libraries(iothub_client_dll
            "-Wl,--exclude-libs,libparson.a"
        )
    endif()

    set(install_staticlibs ${install_staticlibs} iothub_client_dll)
endif()

add_library(iothub_client
    ${iothub_client_c_files}
    ${iothub_client_h_files}
)
setSdkTargetBuildProperties(iothub_client)
target_link_libraries(iothub_client ${iothub_client_libs})
target_link_libraries(iothub_client parson)

if (${use_prov_client_core})
    target_link_libraries(iothub_client hsm_security_client prov_auth_client)
endif()

linkSharedUtil(iothub_client)
set(iothub_client_libs
    iothub_client
    ${iothub_client_libs}
)
# Don't build samples under Win32 ARM for now
if(NOT ${skip_samples})
if(WIN32)
# Build samples for WEC 2013 for both ARM and x86 processsor types
  if(WINCE)
    add_subdirectory(samples)
  else()
    if (NOT ${ARCHITECTURE} STREQUAL "ARM")
    add_subdirectory(samples)
    endif()
  endif()

else()
    add_subdirectory(samples)
endif()
endif()

if(NOT IN_OPENWRT)
    # Disable tests for OpenWRT
    add_subdirectory(tests)
endif()

if(${use_installed_dependencies})

    if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
        set(CMAKE_INSTALL_LIBDIR "lib")
    endif()

    if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
        set(CMAKE_INSTALL_INCLUDEDIR "include")
    endif()

    install(TARGETS ${iothub_client_libs} EXPORT azure_iot_sdksTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/../bin
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot
    )
    install(FILES ${iothub_client_h_install_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot)
else()
    install(FILES ${iothub_client_h_install_files}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot)
    install(TARGETS ${install_staticlibs}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
