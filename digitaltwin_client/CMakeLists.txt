#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for digitaltwin_client

if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

compileAsC99()

set(digitaltwin_client_c_files
    ./src/dt_lock_thread_binding_impl.c
    ./src/dt_lock_thread_binding_stub.c
    ./src/dt_client_core.c
    ./src/dt_device.c
    ./src/dt_device_ll.c
    ./src/dt_interface_client.c
    ./src/dt_interface_list.c
  	./src/dt_model_definition.c
    ./src/dt_version.c
)

set(digitaltwin_client_h_files
    ./inc/digitaltwin_client_common.h 
    ./inc/digitaltwin_client_version.h
    ./inc/digitaltwin_device_client.h
    ./inc/digitaltwin_device_client_ll.h
    ./inc/digitaltwin_interface_client.h
  	./inc/digitaltwin_model_definition.h
    ./inc/internal/dt_interface_private.h
    ./inc/internal/dt_interface_list.h
    ./inc/internal/dt_lock_thread_binding.h
    ./inc/internal/dt_lock_thread_binding_impl.h
    ./inc/internal/dt_lock_thread_binding_stub.h
    ./inc/internal/dt_raw_interface.h
)

set(digitaltwin_client_libs)

set(install_staticlibs
    digitaltwin_client
)

set(digitaltwin_client_h_install_files
    ${digitaltwin_client_h_files}
)

set(DIGITALTWIN_CLIENT_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc ${CMAKE_CURRENT_LIST_DIR}/inc/internal CACHE INTERNAL "this is what needs to be included if using digitaltwin_client lib" FORCE)

include_directories(../deps/parson)
include_directories(${AZURE_C_SHARED_UTILITY_INCLUDES})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${DIGITALTWIN_CLIENT_INC_FOLDER})

if(${memory_trace})
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
endif()


IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

add_definitions(-DDIGITALTWIN_LOGGING_ENABLED)


add_library(digitaltwin_client
    ${digitaltwin_client_c_files}
    ${digitaltwin_client_h_files}
)
setSdkTargetBuildProperties(digitaltwin_client)
target_link_libraries(digitaltwin_client parson)

linkSharedUtil(digitaltwin_client)
set(digitaltwin_client_libs
    digitaltwin_client
    ${digitaltwin_client_libs}
)
add_subdirectory(tests)

if(NOT ${skip_samples})
    add_subdirectory(samples)
endif()

if (${build_as_dynamic})
    set(digitaltwin_def_file ./src/digitaltwin_client_dll.def)
    
    add_library(digitaltwin_client_dll SHARED
        ${digitaltwin_client_c_files}
        ${digitaltwin_client_h_files}
        ${digitaltwin_def_file}
    )
    
    setSdkTargetBuildProperties(digitaltwin_client_dll)
    target_link_libraries(digitaltwin_client_dll ${digitaltwin_client_libs} iothub_client_dll)
    if(NOT WIN32)
        set_target_properties(digitaltwin_client_dll PROPERTIES OUTPUT_NAME "digitaltwin_client")
    endif()
    
    set_target_properties(digitaltwin_client_dll PROPERTIES
        ARCHIVE_OUTPUT_NAME "digitaltwin_client_dll_import"
        ENABLE_EXPORTS YES
        VERSION ${IOT_SDK_VERSION}
        SOVERSION ${IOT_SDK_VERION_MAJOR}
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    
    target_link_libraries(digitaltwin_client_dll parson)
    
    if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
        target_link_libraries(digitaltwin_client_dll
            "-Wl,--exclude-libs,libparson.a"
        )
    endif()
    
    set(install_staticlibs ${install_staticlibs} digitaltwin_client_dll)
endif()

if(${use_installed_dependencies})
    if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
        set(CMAKE_INSTALL_LIBDIR "lib")
    endif()

    if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
        set(CMAKE_INSTALL_INCLUDEDIR "include")
    endif()

    install(TARGETS ${digitaltwin_client_libs} EXPORT azure_iot_sdksTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot
    )
    install(FILES ${digitaltwin_client_h_install_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot)
else()
	install(FILES ${digitaltwin_client_h_install_files}
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot)
	install(TARGETS ${install_staticlibs}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
