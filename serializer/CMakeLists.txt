#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists for serializer

option(use_floats "if use_floats is used, serializer will use floats in its typesystem" ON)

if(${use_floats})
else()
    add_definitions(-DNO_FLOATS)
endif()

if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

compileAsC99()

set(serializer_c_files
    ./src/agenttypesystem.c
    ./src/codefirst.c
    ./src/commanddecoder.c
    ./src/datamarshaller.c
    ./src/datapublisher.c
    ./src/dataserializer.c
    ./src/iotdevice.c
    ./src/jsondecoder.c
    ./src/jsonencoder.c
    ./src/makefile
    ./src/multitree.c
    ./src/schema.c
    ./src/schemalib.c
    ./src/schemaserializer.c
    ./src/methodreturn.c
)

set(serializer_h_files
    ./inc/agenttypesystem.h
    ./inc/codefirst.h
    ./inc/commanddecoder.h
    ./inc/datamarshaller.h
    ./inc/datapublisher.h
    ./inc/dataserializer.h
    ./inc/iotdevice.h
    ./inc/jsondecoder.h
    ./inc/jsonencoder.h
    ./inc/multitree.h
    ./inc/schema.h
    ./inc/schemalib.h
    ./inc/schemaserializer.h
    ./inc/serializer.h
    ./inc/serializer_devicetwin.h
    ./inc/methodreturn.h
)

#these are the include folders
#the following "set" statement exports across the project a global variable called SHARED_UTIL_INC_FOLDER that expands to whatever needs to included when using COMMON library
set(SERIALIZER_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using serializer lib" FORCE)

include_directories(../deps/parson)
include_directories(${SERIALIZER_INC_FOLDER} ${SHARED_UTIL_INC_FOLDER})
include_directories(${AZURE_C_SHARED_UTILITY_INCLUDES})

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

add_library(serializer ${serializer_c_files} ${serializer_h_files})
target_link_libraries(serializer parson)

set (install_libs serializer)

if (${build_as_dynamic})
    add_library(
        serializer_dll SHARED ${serializer_c_files} ${serializer_h_files} ./src/serializer.def
    )
    target_link_libraries(serializer_dll
        aziotsharedutil_dll
        parson
    )

    if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
        target_link_libraries(serializer_dll
            "-Wl,--exclude-libs,libparson.a"
        )
    endif()

    set_target_properties(serializer_dll PROPERTIES
        OUTPUT_NAME "serializer"
        ARCHIVE_OUTPUT_NAME "serializer_dll_import"
        ENABLE_EXPORTS YES
        WINDOWS_EXPORT_ALL_SYMBOLS YES
        PDB_NAME "serializer_dll"
        VERSION ${IOT_SDK_VERSION}
        SOVERSION ${IOT_SDK_VERION_MAJOR}
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    set (install_libs ${install_libs} serializer_dll)
endif()
setSdkTargetBuildProperties(serializer)

if (NOT ${skip_samples})
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
    if(${run_unittests})
        add_subdirectory(tests)
    endif()
endif()

if(${use_installed_dependencies})
    include(GNUInstallDirs)

    #Set CMAKE_INSTALL_LIBDIR if not defined
    if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
        set(CMAKE_INSTALL_LIBDIR "lib")
    endif()

    install(TARGETS serializer EXPORT azure_iot_sdksTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/../bin
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot
    )
    install(FILES ${serializer_h_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot)
else()
    install(FILES ${serializer_h_files}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot)
    install(TARGETS ${install_libs}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
