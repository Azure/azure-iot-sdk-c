#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for iothub_client

if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

compileAsC99()

set(iothub_client_ll_transport_c_files
    ./src/version.c
    ./src/iothub_client_authorization.c
    ./src/iothub_message.c
    ./src/iothub_client_diagnostic.c
 )

set(iothub_client_libs)

set(install_staticlibs
    iothub_client
)

add_library(parson
    ../deps/parson/parson.c
)

if(MSVC)
    set_source_files_properties(../deps/parson/parson.c PROPERTIES COMPILE_FLAGS "/wd4244 /wd4232")
endif()

set(install_staticlibs ${install_staticlibs}
    parson
)

set(iothub_client_libs ${iothub_client_libs}
    parson
)

if(NOT dont_use_uploadtoblob)
    set(iothub_client_ll_transport_c_files
        ${iothub_client_ll_transport_c_files}
        ./src/iothub_client_ll_uploadtoblob.c
        ./src/blob.c
    )

    set(iothub_client_ll_transport_h_files
        ${iothub_client_ll_transport_h_files}
        ./inc/blob.h
    )
endif()

set(iothub_client_ll_transport_h_files
    ./inc/iothub_client_authorization.h
    ./inc/iothub_message.h
    ./inc/iothub_client_version.h
    ./inc/iothub_transport_ll.h
    ./inc/blob.h
    ./inc/iothub_client_diagnostic.h
)

if (${use_prov_client})
    set(iothub_client_ll_transport_h_files
        ${iothub_client_ll_transport_h_files}
        ./inc/iothub_client_hsm_ll.h
    )
    set(iothub_client_ll_transport_c_files
        ${iothub_client_ll_transport_c_files}
    )
endif()

if(NOT ${dont_use_uploadtoblob})
    set(iothub_client_ll_transport_h_files
        ${iothub_client_ll_transport_h_files}
        ./inc/iothub_client_ll_uploadtoblob.h
    )
endif()

set(iothub_client_c_files
    ./src/iothub_client.c
	./src/iothub_client_ll.c
    ./src/version.c
    ./src/iothubtransport.c
)

set(iothub_client_h_files
    ./inc/iothub_client.h
	./inc/iothub_client_ll.h
    ./inc/iothub_client_options.h
    ./inc/iothub_client_version.h
    ./inc/iothubtransport.h
    ./inc/iothub_client_private.h
)

set(iothub_client_h_install_files
    ${iothub_client_ll_transport_h_files}
    ${iothub_client_h_files}
)

if(${use_http})
    set(install_staticlibs ${install_staticlibs}
        iothub_client_http_transport
    )
    set(iothub_client_http_transport_c_files
        ${iothub_client_ll_transport_c_files}
        ./src/iothubtransporthttp.c
    )

    set(iothub_client_http_transport_h_files
        ${iothub_client_ll_transport_h_files}
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
        ${iothub_client_ll_transport_c_files}
        ./src/iothubtransport_amqp_common.c
        ./src/iothubtransport_amqp_device.c
        ./src/iothubtransport_amqp_cbs_auth.c
        ./src/iothubtransport_amqp_connection.c
        ./src/iothubtransport_amqp_telemetry_messenger.c
        ./src/iothubtransport_amqp_twin_messenger.c
        ./src/iothubtransport_amqp_messenger.c
        ./src/iothubtransportamqp_methods.c
        ./src/iothub_client_retry_control.c
        ./src/message_queue.c
        ./src/uamqp_messaging.c
    )

    set(iothub_client_amqp_transport_common_h_files
        ${iothub_client_ll_transport_h_files}
        ./inc/iothubtransport_amqp_common.h
        ./inc/iothubtransport_amqp_device.h
        ./inc/iothubtransport_amqp_cbs_auth.h
        ./inc/iothubtransport_amqp_connection.h
        ./inc/iothubtransport_amqp_telemetry_messenger.h
        ./inc/iothubtransport_amqp_twin_messenger.h
        ./inc/iothubtransport_amqp_messenger.h
        ./inc/iothubtransportamqp_methods.h
        ./inc/iothub_client_retry_control.h
        ./inc/message_queue.h
        ./inc/uamqp_messaging.h
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
        ${iothub_client_ll_transport_c_files}
        ./src/iothubtransport_mqtt_common.c
        ./src/iothub_client_retry_control.c
        ./src/iothubtransportmqtt_websockets.c
    )
    set(iothub_client_mqtt_ws_transport_h_files
        ${iothub_client_ll_transport_h_files}
        ./inc/iothubtransport_mqtt_common.h
        ./inc/iothub_client_retry_control.h
        ./inc/iothubtransportmqtt_websockets.h
    )

    set(iothub_client_mqtt_transport_c_files
        ${iothub_client_ll_transport_c_files}
        ./src/iothubtransport_mqtt_common.c
        ./src/iothub_client_retry_control.c
        ./src/iothubtransportmqtt.c
    )

    set(iothub_client_mqtt_transport_h_files
        ${iothub_client_ll_transport_h_files}
        ./inc/iothubtransport_mqtt_common.h
        ./inc/iothub_client_retry_control.h
        ./inc/iothubtransportmqtt.h
    )

    set(iothub_client_h_install_files
        ${iothub_client_h_install_files}
        ${iothub_client_mqtt_transport_h_files}
        ${iothub_client_mqtt_ws_transport_h_files}
    )
endif()

#these are the include folders
#the following "set" statement exports across the project a global variable called SHARED_UTIL_INC_FOLDER that expands to whatever needs to included when using COMMON library

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


if(NOT ${dont_use_uploadtoblob})
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

if(dont_use_uploadtoblob)
	set(upload "_no_upload")
else()
	set(upload "")
endif()

if(${use_http})
    include_directories(${IOTHUB_CLIENT_HTTP_TRANSPORT_INC_FOLDER})
    add_library(iothub_client_http_transport
        ${iothub_client_http_transport_c_files}
        ${iothub_client_http_transport_h_files}
	)
    linkSharedUtil(iothub_client_http_transport)
    set(iothub_client_libs
        ${iothub_client_libs}
        iothub_client_http_transport
    )

    if(build_as_dynamic)
		set(transport "http")
		
		configure_file(./src/iothub_client_transport${upload}_template.def ${CMAKE_CURRENT_SOURCE_DIR}/src/iothub_client_${transport}_transport_dll_temp.def @ONLY)
		add_library(iothub_client_http_transport_dll SHARED
            ${iothub_client_http_transport_c_files}
            ${iothub_client_http_transport_h_files}
			./src/iothub_client_${transport}_transport_dll_temp.def
		)
        linkSharedUtil(iothub_client_http_transport_dll)

        set_target_properties(iothub_client_http_transport_dll PROPERTIES
            OUTPUT_NAME "iothub_client_http_transport"
            ARCHIVE_OUTPUT_NAME "iothub_client_http_transport_dll_import"
            ENABLE_EXPORTS YES
            VERSION ${IOT_SDK_VERSION}
            SOVERSION ${IOT_SDK_VERION_MAJOR}
            BUILD_WITH_INSTALL_RPATH TRUE
        )
		target_link_libraries(iothub_client_http_transport_dll iothub_client parson)
		
		if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
			target_link_libraries(iothub_client_http_transport_dll
				"-Wl,--exclude-libs,libparson.a"
			)
		endif()

        set(install_staticlibs ${install_staticlibs} iothub_client_http_transport_dll)
    endif(build_as_dynamic)
endif()

if(${use_amqp})
    include_directories(${UAMQP_INCLUDES})
    include_directories(${IOTHUB_CLIENT_AMQP_TRANSPORT_INC_FOLDER} ${UAMQP_INC_FOLDER})
    add_library(iothub_client_amqp_transport
        ${iothub_client_amqp_transport_c_files}
        ${iothub_client_amqp_transport_h_files}
    )
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
    linkSharedUtil(iothub_client_amqp_ws_transport)
    set(iothub_client_libs
        ${iothub_client_libs}
        iothub_client_amqp_ws_transport
    )

    if (build_as_dynamic)
		set(transport "amqp")
		configure_file(./src/iothub_client_transport${upload}_template.def ${CMAKE_CURRENT_SOURCE_DIR}/src/iothub_client_${transport}_transport_dll_temp.def @ONLY)
        add_library(iothub_client_amqp_transport_dll SHARED
            ${iothub_client_amqp_transport_c_files}
            ${iothub_client_amqp_transport_h_files}
			./src/iothub_client_${transport}_transport_dll_temp.def
        )
        linkSharedUtil(iothub_client_amqp_transport_dll)
        target_link_libraries(iothub_client_amqp_transport_dll iothub_client parson uamqp)
		
		if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
			target_link_libraries(iothub_client_amqp_transport_dll
				"-Wl,--exclude-libs,libparson.a"
			)
		endif()

        set_target_properties(iothub_client_amqp_transport_dll PROPERTIES
            OUTPUT_NAME "iothub_client_amqp_transport"
            ARCHIVE_OUTPUT_NAME "iothub_client_amqp_transport_dll_import"
            ENABLE_EXPORTS YES
            VERSION ${IOT_SDK_VERSION}
            SOVERSION ${IOT_SDK_VERION_MAJOR}
            BUILD_WITH_INSTALL_RPATH TRUE
        )
        set(install_staticlibs ${install_staticlibs} iothub_client_amqp_transport_dll)

		set(transport "amqp_ws")
		configure_file(./src/iothub_client_transport${upload}_template.def ${CMAKE_CURRENT_SOURCE_DIR}/src/iothub_client_${transport}_transport_dll_temp.def @ONLY)
        add_library(iothub_client_amqp_ws_transport_dll SHARED
            ${iothub_client_amqp_ws_transport_c_files}
            ${iothub_client_amqp_ws_transport_h_files}
			./src/iothub_client_${transport}_transport_dll_temp.def
        )
        linkSharedUtil(iothub_client_amqp_ws_transport_dll)
        target_link_libraries(iothub_client_amqp_ws_transport_dll iothub_client parson uamqp)
		
		if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
			target_link_libraries(iothub_client_amqp_ws_transport_dll
				"-Wl,--exclude-libs,libparson.a"
			)
		endif()

        set_target_properties(iothub_client_amqp_ws_transport_dll PROPERTIES
            OUTPUT_NAME "iothub_client_amqp_ws_transport"
            ARCHIVE_OUTPUT_NAME "iothub_client_amqp_ws_transport_dll_import"
            ENABLE_EXPORTS YES
            VERSION ${IOT_SDK_VERSION}
            SOVERSION ${IOT_SDK_VERION_MAJOR}
            BUILD_WITH_INSTALL_RPATH TRUE
        )
        set(install_staticlibs ${install_staticlibs} iothub_client_amqp_ws_transport_dll)
    endif(build_as_dynamic)
endif()

if(${use_mqtt})
    include_directories(${UMQTT_INCLUDES})
    include_directories(${IOTHUB_CLIENT_MQTT_TRANSPORT_INC_FOLDER} ${MQTT_INC_FOLDER})
    add_library(iothub_client_mqtt_transport
        ${iothub_client_mqtt_transport_c_files}
        ${iothub_client_mqtt_transport_h_files}
    )
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
    linkSharedUtil(iothub_client_mqtt_ws_transport)
    linkMqttLibrary(iothub_client_mqtt_ws_transport)
    set(iothub_client_libs
        ${iothub_client_libs}
        iothub_client_mqtt_ws_transport
    )

    if (build_as_dynamic)
		set(transport "mqtt")
		configure_file(./src/iothub_client_transport${upload}_template.def ${CMAKE_CURRENT_SOURCE_DIR}/src/iothub_client_${transport}_transport_dll_temp.def @ONLY)
        add_library(iothub_client_mqtt_transport_dll SHARED
            ${iothub_client_mqtt_transport_c_files}
            ${iothub_client_mqtt_transport_h_files}
			./src/iothub_client_${transport}_transport_dll_temp.def
        )
        linkSharedUtil(iothub_client_mqtt_transport_dll)
		linkMqttLibrary(iothub_client_mqtt_transport_dll)
		target_link_libraries(iothub_client_mqtt_transport_dll iothub_client parson)
		
		if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
			target_link_libraries(iothub_client_mqtt_transport_dll
				"-Wl,--exclude-libs,libparson.a"
			)
		endif()
		
        set_target_properties(iothub_client_mqtt_transport_dll PROPERTIES
            OUTPUT_NAME "iothub_client_mqtt_transport"
            ARCHIVE_OUTPUT_NAME "iothub_client_mqtt_transport_dll_import"
            ENABLE_EXPORTS YES
            VERSION ${IOT_SDK_VERSION}
            SOVERSION ${IOT_SDK_VERION_MAJOR}
            BUILD_WITH_INSTALL_RPATH TRUE
        )
        set(install_staticlibs ${install_staticlibs} iothub_client_mqtt_transport_dll)

		set(transport "mqtt_ws")
		configure_file(./src/iothub_client_transport${upload}_template.def ${CMAKE_CURRENT_SOURCE_DIR}/src/iothub_client_${transport}_transport_dll_temp.def @ONLY)
        add_library(iothub_client_mqtt_ws_transport_dll SHARED
            ${iothub_client_mqtt_ws_transport_c_files}
            ${iothub_client_mqtt_ws_transport_h_files}
			./src/iothub_client_${transport}_transport_dll_temp.def
        )
        linkSharedUtil(iothub_client_mqtt_ws_transport_dll)
		linkMqttLibrary(iothub_client_mqtt_ws_transport_dll)
		target_link_libraries(iothub_client_mqtt_ws_transport_dll iothub_client parson)
		
		if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
			target_link_libraries(iothub_client_mqtt_ws_transport_dll
				"-Wl,--exclude-libs,libparson.a"
			)
		endif()

        set_target_properties(iothub_client_mqtt_ws_transport_dll PROPERTIES
            OUTPUT_NAME "iothub_client_mqtt_ws_transport"
            ARCHIVE_OUTPUT_NAME "iothub_client_mqtt_ws_transport_dll_import"
            ENABLE_EXPORTS YES
            VERSION ${IOT_SDK_VERSION}
            SOVERSION ${IOT_SDK_VERION_MAJOR}
            BUILD_WITH_INSTALL_RPATH TRUE
        )
        set(install_staticlibs ${install_staticlibs} iothub_client_mqtt_ws_transport_dll)
    endif(build_as_dynamic)
endif()

include_directories(${IOTHUB_CLIENT_INC_FOLDER})

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

if(${dont_use_uploadtoblob})
    set(iothub_def_file ./src/iothub_client_dll_no_upload.def)
else()
    set(iothub_def_file ./src/iothub_client_dll.def)
endif()


add_library(iothub_client
    ${iothub_client_c_files}
    ${iothub_client_h_files}
)

target_link_libraries(iothub_client parson)

if (${use_prov_client})
    target_link_libraries(iothub_client hsm_security_client prov_auth_client)
endif()

linkSharedUtil(iothub_client)
set(iothub_client_libs
    iothub_client
    ${iothub_client_libs}
)

# if (${build_as_dynamic})
	# add_library(iothub_client_dll SHARED
		# ${iothub_client_c_files}
		# ${iothub_client_h_files}
		# ${iothub_def_file}
	# )

	# target_link_libraries(iothub_client_dll parson)
		
	# if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
		# target_link_libraries(iothub_client_dll
			# "-Wl,--exclude-libs,libparson.a"
		# )
	# endif()

	# if (${use_prov_client})
		# target_link_libraries(iothub_client_dll hsm_security_client prov_auth_client)
	# endif()

	# set_target_properties(iothub_client_dll PROPERTIES
		# OUTPUT_NAME "iothub_client"
		# ARCHIVE_OUTPUT_NAME "iothub_client_dll_import"
		# ENABLE_EXPORTS YES
		# VERSION ${IOT_SDK_VERSION}
		# SOVERSION ${IOT_SDK_VERION_MAJOR}
		# BUILD_WITH_INSTALL_RPATH TRUE
	# )

	# linkSharedUtil(iothub_client_dll)
	# set(iothub_client_libs
		# iothub_client_dll
		# ${iothub_client_libs}
# )
	
# endif(${build_as_dynamic})

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
