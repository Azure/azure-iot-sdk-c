#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)
#this is CMakeLists for iothub_client

if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

compileAsC99()

set(iothub_client_c_files
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_core.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_core_ll.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_diagnostic.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_ll.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub_device_client.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub_device_client_ll.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub_message.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub_module_client.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothub_module_client_ll.c
    ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransport.c
    ${CMAKE_CURRENT_LIST_DIR}/src/version.c
)

set(iothub_client_h_files
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_client_core.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_client_core_ll.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_client.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_client_core_common.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_client_ll.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_diagnostic.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_internal_consts.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_client_options.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_private.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_client_version.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_device_client.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_device_client_ll.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_module_client.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_module_client_ll.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_transport_ll.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_message.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_message_private.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransport.h
)

set(iothub_client_libs)

set(install_staticlibs
    iothub_client
)

if(NOT dont_use_uploadtoblob)
    set(iothub_client_c_files
        ${iothub_client_c_files}
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_ll_uploadtoblob.c
        ${CMAKE_CURRENT_LIST_DIR}/src/blob.c
    )

    set(iothub_client_h_files
        ${iothub_client_h_files}
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/blob.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_ll_uploadtoblob.h
    )
endif()

if (use_edge_modules)
    set(iothub_client_c_files
        ${iothub_client_c_files}
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_edge.c
    )

    set (iothub_client_h_files
        ${iothub_client_h_files}
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_edge.h
    )
endif()

#this is around for back compat only
if (${use_prov_client_core})
    set(iothub_client_h_files
        ${iothub_client_h_files}
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_hsm_ll.h
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
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_authorization.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_retry_control.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_transport_ll_private.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransporthttp.c
    )

    set(iothub_client_http_transport_h_files
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_authorization.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_retry_control.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_transport_ll_private.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/iothubtransporthttp.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/iothub_transport_ll.h
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
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_authorization.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_retry_control.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_transport_ll_private.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransport_amqp_common.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransport_amqp_device.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransport_amqp_cbs_auth.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransport_amqp_connection.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransport_amqp_telemetry_messenger.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransport_amqp_twin_messenger.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransport_amqp_messenger.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransportamqp_methods.c
        ${CMAKE_CURRENT_LIST_DIR}/src/message_queue.c
        ${CMAKE_CURRENT_LIST_DIR}/src/uamqp_messaging.c
    )

    set(iothub_client_amqp_transport_common_h_files
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_authorization.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_retry_control.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_transport_ll_private.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransport_amqp_common.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransport_amqp_device.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransport_amqp_cbs_auth.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransport_amqp_connection.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransport_amqp_telemetry_messenger.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransport_amqp_twin_messenger.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransport_amqp_messenger.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransportamqp_methods.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/message_queue.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/uamqp_messaging.h
    )

    set(iothub_client_amqp_transport_c_files
        ${iothub_client_amqp_transport_common_c_files}
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransportamqp.c
    )

    set(iothub_client_amqp_transport_h_files
        ${iothub_client_amqp_transport_common_h_files}
        ${CMAKE_CURRENT_LIST_DIR}/inc/iothubtransportamqp.h
    )

    set(iothub_client_h_install_files
        ${iothub_client_h_install_files}
        ${iothub_client_amqp_transport_h_files}
    )

    set(iothub_client_amqp_ws_transport_c_files
        ${iothub_client_amqp_transport_common_c_files}
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransportamqp_websockets.c
    )

    set(iothub_client_amqp_ws_transport_h_files
        ${iothub_client_amqp_transport_common_h_files}
        ${CMAKE_CURRENT_LIST_DIR}/inc/iothubtransportamqp_websockets.h
    )

    set(iothub_client_h_install_files
        ${iothub_client_h_install_files}
        ${CMAKE_CURRENT_LIST_DIR}/inc/iothubtransportamqp_websockets.h
    )
endif()

if(${use_mqtt})
    set(install_staticlibs ${install_staticlibs}
        iothub_client_mqtt_transport
        iothub_client_mqtt_ws_transport
    )
    set(iothub_client_mqtt_ws_transport_c_files
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_authorization.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_retry_control.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_transport_ll_private.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransport_mqtt_common.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransportmqtt_websockets.c
    )
    set(iothub_client_mqtt_ws_transport_h_files
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_authorization.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_retry_control.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_transport_ll_private.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransport_mqtt_common.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/iothubtransportmqtt_websockets.h
    )

    set(iothub_client_mqtt_transport_c_files
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_authorization.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_retry_control.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothub_transport_ll_private.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransport_mqtt_common.c
        ${CMAKE_CURRENT_LIST_DIR}/src/iothubtransportmqtt.c
    )

    set(iothub_client_mqtt_transport_h_files
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_authorization.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_client_retry_control.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothub_transport_ll_private.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/internal/iothubtransport_mqtt_common.h
        ${CMAKE_CURRENT_LIST_DIR}/inc/iothubtransportmqtt.h
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


if(NOT ${dont_use_uploadtoblob} OR ${use_edge_modules} AND NOT ${use_installed_dependencies})
    include_directories(${CMAKE_CURRENT_LIST_DIR}/../deps/parson)
endif()

include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${AZURE_C_SHARED_UTILITY_INCLUDES})
include_directories(${SHARED_UTIL_INC_FOLDER})

include_directories(${IOTHUB_CLIENT_INC_FOLDER})

if(${memory_trace})
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
endif()

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

set(iothub_transport_source)

if(${use_http})
    include_directories(${IOTHUB_CLIENT_HTTP_TRANSPORT_INC_FOLDER})
    add_library(iothub_client_http_transport
        ${iothub_client_http_transport_c_files}
        ${iothub_client_http_transport_h_files}
    )
    applyXcodeBuildFlagsIfNeeded(iothub_client_http_transport)
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
    applyXcodeBuildFlagsIfNeeded(iothub_client_amqp_transport)
    linkSharedUtil(iothub_client_amqp_transport)
    linkUAMQP(iothub_client_amqp_transport)
    set(iothub_client_libs
        ${iothub_client_libs}
        iothub_client_amqp_transport
    )

    include_directories(${iothub_client_amqp_ws_transport_INC_FOLDER} ${UAMQP_INC_FOLDER})
    add_library(iothub_client_amqp_ws_transport
        ${iothub_client_amqp_ws_transport_c_files}
        ${iothub_client_amqp_ws_transport_h_files}
    )
    applyXcodeBuildFlagsIfNeeded(iothub_client_amqp_ws_transport)
    linkSharedUtil(iothub_client_amqp_ws_transport)
    linkUAMQP(iothub_client_amqp_ws_transport)
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
    endif(build_as_dynamic)
endif()

if(${use_mqtt})
    include_directories(${UMQTT_INCLUDES})
    include_directories(${IOTHUB_CLIENT_MQTT_TRANSPORT_INC_FOLDER} ${MQTT_INC_FOLDER})
    add_library(iothub_client_mqtt_transport
        ${iothub_client_mqtt_transport_c_files}
        ${iothub_client_mqtt_transport_h_files}
    )
    applyXcodeBuildFlagsIfNeeded(iothub_client_mqtt_transport)
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
    applyXcodeBuildFlagsIfNeeded(iothub_client_mqtt_ws_transport)
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
    set(iothub_def_file ${CMAKE_CURRENT_LIST_DIR}/src/iothub_client_dll.def)
    if(NOT ${dont_use_uploadtoblob})
        set(iothub_def_file ${iothub_def_file} ${CMAKE_CURRENT_LIST_DIR}/src/upload_to_blob.def)
    endif()

    if(use_amqp)
        set(iothub_def_file ${iothub_def_file} ${CMAKE_CURRENT_LIST_DIR}/src/iothub_transport_amqp.def)
    endif()

    if(use_http)
        set(iothub_def_file ${iothub_def_file} ${CMAKE_CURRENT_LIST_DIR}/src/iothub_transport_http.def)
    endif()

    if(use_mqtt)
        set(iothub_def_file ${iothub_def_file} ${CMAKE_CURRENT_LIST_DIR}/src/iothub_transport_mqtt.def)
    endif()

    if (${use_edge_modules})
        set(iothub_def_file ${iothub_def_file} ${CMAKE_CURRENT_LIST_DIR}/src/iothub_edge_modules.def)
    endif()

    add_library(iothub_client_dll SHARED
        ${iothub_client_c_files}
        ${iothub_client_h_files}
        ${iothub_def_file}
        ${iothub_transport_source}
    )

    applyXcodeBuildFlagsIfNeeded(iothub_client_dll)
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
target_include_directories(iothub_client
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/azureiot/include>
)
applyXcodeBuildFlagsIfNeeded(iothub_client)
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
    if (NOT ${ARCHITECTURE} STREQUAL "ARM")
        add_subdirectory(samples)
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
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
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
