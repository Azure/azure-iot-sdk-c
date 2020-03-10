# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
project(azure_iot_sdks)

include("${CMAKE_CURRENT_LIST_DIR}/configs/azure_iot_sdksFunctions.cmake")

getIoTSDKVersion()
message(STATUS "IoT Client SDK Version = ${IOT_SDK_VERSION}")

if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

#
#making a global variable to know if we are on linux, windows, or macosx.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WINDOWS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
    # on Linux, enable valgrind
    # these commands (MEMORYCHECK...) need to apear BEFORE include(CTest) or they will not have any effect
    find_program(MEMORYCHECK_COMMAND valgrind)
    set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=1")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(MACOSX TRUE)
    add_definitions(-DMACOSX)
endif()


include (CTest)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /wd4232")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /wd4232")
    # Make warning as error
    add_definitions(/WX)
else()
    # Make warning as error
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif()

IF(WIN32)
    # windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(hsm_type_x509 OFF CACHE BOOL "x509 type of hsm used with the Provisioning client")
set(hsm_type_sastoken OFF CACHE BOOL "tpm type of hsm used with the Provisioning client")
set(hsm_type_symm_key OFF CACHE BOOL "Symmetric key type of hsm used with the Provisioning client")
set(hsm_type_custom OFF CACHE BOOL "hsm type of custom used with the Provisioning client")
set(hsm_custom_lib "" CACHE STRING "Full path to custom HSM repo library")
set(run_sfc_tests OFF CACHE BOOL "setup the Service Fault tests")

# the following variables are project-wide and can be used with cmake-gui
option(use_amqp "set use_amqp to ON if amqp is to be used, set to OFF to not use amqp" ON)
option(use_http "set use_http to ON if http is to be used, set to OFF to not use http" ON)
option(use_mqtt "set use_mqtt to ON if mqtt is to be used, set to OFF to not use mqtt" ON)
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF)" OFF)
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(run_longhaul_tests "set run_longhaul_tests to ON to run longhaul tests (default is OFF)[if possible, they are always build]" OFF)
option(skip_samples "set skip_samples to ON to skip building samples (default is OFF)[if possible, they are always build]" OFF)
option(build_service_client "controls whether the iothub_service_client is built or not" ON)
option(build_provisioning_service_client "controls whether the provisioning_service_client is built or not" ON)
option(build_python "builds the Python native iothub_client module" OFF)
option(dont_use_uploadtoblob "set dont_use_uploadtoblob to ON if the functionality of upload to blob is to be excluded, OFF otherwise. It requires HTTP" OFF)
option(no_logging "disable logging" OFF)
option(use_installed_dependencies "set use_installed_dependencies to ON to use installed packages instead of building dependencies from submodules" OFF)
option(build_as_dynamic "build the IoT SDK libaries as dynamic"  OFF)
option(use_prov_client "Enable provisioning client" OFF)
option(use_tpm_simulator "tpm simulator type of hsm used with the provisioning client" OFF)
option(use_edge_modules "Enable support for running modules against Azure IoT Edge" OFF)
option(use_custom_heap "use externally defined heap functions instead of the malloc family" OFF)
option(use_baltimore_cert "set use_baltimore_cert to ON if the Baltimore cert is to be used, set to OFF to not use it" OFF)
option(use_microsoftazure_de_cert "set use_microsoftazure_de_cert to ON if the MicrosoftAzure DE cert is to be used, set to OFF to not use it" OFF)
option(use_portal_azure_cn_cert "set use_portal_azure_cn_cert to ON if the Portal Azure CN cert is to be used, set to OFF to not use it" OFF)

set(compileOption_C "" CACHE STRING "passes a string to the command line of the C compiler")
set(compileOption_CXX "" CACHE STRING "passes a string to the command line of the C++ compiler")
set(linkerOption "" CACHE STRING "passes a string to the shared and exe linker options of the C compiler")

set(use_prov_client_core OFF)

if(${use_custom_heap})
    add_definitions(-DGB_USE_CUSTOM_HEAP)
endif()

if (NOT ${use_amqp} AND NOT ${use_http} AND NOT ${use_mqtt})
    message(FATAL_ERROR "CMAKE Failure: AMQP, HTTP & MQTT are all disable, iothub client must have one protocol enabled")
endif()

if (XCODE AND ${use_prov_client})
    # The TPM module is not available on Mac, and Mac's <string.h> and <unistd.h> files collide as well
    message(FATAL_ERROR "Provisioning client is not supported on Mac")
endif()

if (WIN32 OR MACOSX)
    option(use_openssl "set use_openssl to ON to use OpenSSL." OFF)
else()
    option(use_openssl "set use_openssl to ON to use OpenSSL." ON)
endif()
option(use_mbedtls "set use_mbedtls to ON to use mbedtls." OFF)
option(use_bearssl "set use_bearssl to ON to use bearssl." OFF)
option(use_wolfssl "set use_bearssl to ON to use bearssl." OFF)

# openssl samples on Windows need to have a trusted cert set
if ((WIN32 AND ${use_openssl}) OR ${use_wolfssl} OR ${use_mbedtls} OR ${use_bearssl})
    option(use_sample_trusted_cert "Set flag in samples to use SDK's built-in CA as TrustedCerts" ON)
else()
    option(use_sample_trusted_cert "Set flag in samples to use SDK's built-in CA as TrustedCerts" OFF)
endif()

# Enable specific certs
if (${use_baltimore_cert})
    add_definitions(-DUSE_BALTIMORE_CERT)
endif()

if (${use_microsoftazure_de_cert})
    add_definitions(-DUSE_MICROSOFTAZURE_DE_CERT)
endif()

if (${use_portal_azure_cn_cert})
    add_definitions(-DUSE_PORTAL_AZURE_CN_CERT)
endif()

# Enable IoT SDK to act as a module for Edge
if(${use_edge_modules})
    set(use_prov_client_core ON)
    set(use_http ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_EDGE_MODULES")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_EDGE_MODULES")
    set(hsm_type_edge_module ON)
endif()

# Set Provisioning Information.  This will also setup appropriate HSM
if (${use_prov_client})
    set(use_prov_client_core ON)
    if ("${hsm_custom_lib}" STREQUAL "")
        if ((NOT ${hsm_type_x509}) AND (NOT ${hsm_type_sastoken}) AND (NOT ${hsm_type_symm_key}))
            # If the cmake option did not explicitly configure an hsm type, then enable them all.
            set(hsm_type_x509 ON)
            set(hsm_type_sastoken ON)
            set(hsm_type_symm_key ON)
        endif()
    else()
        set(hsm_type_custom ON)
    endif()
endif()

if (${use_prov_client_core})
    getProvSDKVersion()
    message(STATUS "Provisioning SDK Version = ${PROV_SDK_VERSION}")
endif()

# setting nuget_e2e_tests will only generate e2e tests to run with nuget packages.
# Install-packages from Package Manager Console in VS before building the projects
option(nuget_e2e_tests "set nuget_e2e_tests to ON to generate e2e tests to run with nuget packages (default is OFF)" OFF)

# check for conflicting options
if (NOT ${use_http})
    MESSAGE( "Setting dont_use_uploadtoblob to ON because use_http is OFF")
    set(dont_use_uploadtoblob "ON")
    MESSAGE( STATUS "use_http:         " ${use_http} )
    MESSAGE( STATUS "dont_use_uploadtoblob:         " ${dont_use_uploadtoblob} )
endif()

if (${dont_use_uploadtoblob})
    add_definitions(-DDONT_USE_UPLOADTOBLOB)
endif()

if (${no_logging})
    add_definitions(-DNO_LOGGING)
endif()

# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# build the parson library for json parsing
add_library(parson
    ./deps/parson/parson.c
    ./deps/parson/parson.h
)
if (MSVC)
    set_source_files_properties(../deps/parson/parson.c PROPERTIES COMPILE_FLAGS "/wd4244 /wd4232")
endif()
set(parson_h_install_files ./deps/parson/parson.h)
set(parson_install_libs parson)

if (IN_OPENWRT)
    ADD_DEFINITIONS("$ENV{TARGET_LDFLAGS}" "$ENV{TARGET_CPPFLAGS}" "$ENV{TARGET_CFLAGS}")
    INCLUDE_DIRECTORIES("$ENV{TOOLCHAIN_DIR}/usr/include" "$ENV{TARGET_LDFLAGS}" "$ENV{TARGET_CPPFLAGS}" "$ENV{TARGET_CFLAGS}")
endif()

if (LINUX)
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
        # now all static libraries use PIC flag for Python shared lib
        set(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")
    endif()
endif()

# if any compiler has a command line switch called "OFF" then it will need special care
if (NOT "${compileOption_C}" STREQUAL "")
    set(CMAKE_C_FLAGS "${compileOption_C} ${CMAKE_C_FLAGS}")
endif()

if (NOT "${compileOption_CXX}" STREQUAL "")
    set(CMAKE_CXX_FLAGS "${compileOption_CXX} ${CMAKE_CXX_FLAGS}")
endif()

if (NOT "${linkerOption}" STREQUAL "")
    message("linkerOption: ${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${linkerOption}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${linkerOption}")
endif()

include("configs/azure_iot_sdksFunctions.cmake")

# do not add or build any tests of the dependencies
set(original_run_e2e_tests ${run_e2e_tests})
set(original_run_unittests ${run_unittests})
set(original_skip_samples ${skip_samples})

set(run_e2e_tests OFF)
set(run_unittests OFF)
set(skip_samples ON)
set(use_cppunittest ON)

if (NOT ${use_installed_dependencies})
    if (NOT TARGET azure_macro_utils_c AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/deps/azure-macro-utils-c/CMakeLists.txt")
        add_subdirectory(deps/azure-macro-utils-c)
    endif()
    if (NOT TARGET umock_c)
        # Get the repo if it's not there
        add_subdirectory(deps/umock-c)
    endif()

    if (${original_run_e2e_tests} OR ${original_run_unittests} OR ${run_sfc_tests})
        if (NOT TARGET testrunnerswitcher)
            add_subdirectory(deps/azure-c-testrunnerswitcher)
        endif()
        if (NOT TARGET ctest)
            # Get the repo if it's not there
            add_subdirectory(deps/azure-ctest)
        endif()
        enable_testing()
    endif()
else()
    if (NOT azure_macro_utils_cFOUND)
        find_package(azure_macro_utils_c REQUIRED CONFIG)
    endif ()
    if (NOT umock_cFOUND)
        find_package(umock_c REQUIRED CONFIG)
    endif ()
endif()

include_directories(${MACRO_UTILS_INC_FOLDER})
include_directories(${UMOCK_C_INC_FOLDER})

include("dependencies.cmake")
if (${original_run_e2e_tests} OR ${original_run_unittests} OR ${run_sfc_tests})
    # Used for serializer
    add_subdirectory("${SHARED_UTIL_FOLDER}/testtools/sal")
    add_subdirectory("${SHARED_UTIL_FOLDER}/testtools/micromock")
endif()

if (${original_run_e2e_tests} OR ${original_run_unittests} OR ${run_sfc_tests})
    set(SHARED_UTIL_REAL_TEST_FOLDER ${CMAKE_CURRENT_LIST_DIR}/c-utility/tests/real_test_files CACHE INTERNAL "this is what needs to be included when doing test sources" FORCE)
endif()

set_platform_files(${SHARED_UTIL_FOLDER})

set(run_e2e_tests ${original_run_e2e_tests})
set(run_unittests ${original_run_unittests})
set(skip_samples ${original_skip_samples})

# this project uses several other projects that are build not by these CMakeFiles
# this project also targets several OSes

include(CheckSymbolExists)
function(detect_architecture symbol arch)
    if (NOT DEFINED ARCHITECTURE OR ARCHITECTURE STREQUAL "")
        set(CMAKE_REQUIRED_QUIET 1)
        check_symbol_exists("${symbol}" "" ARCHITECTURE_${arch})
        unset(CMAKE_REQUIRED_QUIET)

        # The output variable needs to be unique across invocations otherwise
        # CMake's crazy scope rules will keep it defined
        if (ARCHITECTURE_${arch})
            set(ARCHITECTURE "${arch}" PARENT_SCOPE)
            set(ARCHITECTURE_${arch} 1 PARENT_SCOPE)
            add_definitions(-DARCHITECTURE_${arch}=1)
        endif()
    endif()
endfunction()

if (MSVC)
    detect_architecture("_M_AMD64" x86_64)
    detect_architecture("_M_IX86" x86)
    detect_architecture("_M_ARM" ARM)
else()
    detect_architecture("__x86_64__" x86_64)
    detect_architecture("__i386__" x86)
    detect_architecture("__arm__" ARM)
endif()

if (NOT DEFINED ARCHITECTURE OR ARCHITECTURE STREQUAL "")
    set(ARCHITECTURE "GENERIC")
endif()

message(STATUS "iothub architecture: ${ARCHITECTURE}")

macro(compileAsC99)
    if (CMAKE_VERSION VERSION_LESS "3.1")
        if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
            set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
            if (NOT IN_OPENWRT)
                set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
            endif()
        endif()
    else()
        set (CMAKE_C_STANDARD 99)
        set (CMAKE_CXX_STANDARD 11)
    endif()
endmacro(compileAsC99)

macro(compileAsC11)
    if (CMAKE_VERSION VERSION_LESS "3.1")
        if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
            set (CMAKE_C_FLAGS "--std=c11 ${CMAKE_C_FLAGS}")
            set (CMAKE_C_FLAGS "-D_POSIX_C_SOURCE=200112L ${CMAKE_C_FLAGS}")
            set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
        endif()
    else()
        set (CMAKE_C_STANDARD 11)
        set (CMAKE_CXX_STANDARD 11)
    endif()
endmacro(compileAsC11)

if (WIN32)
    set(LOCK_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/lock_win32.c)
    set(THREAD_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/threadapi_c11.c)
else()
    set(LOCK_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/lock_pthreads.c)
    set(THREAD_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/threadapi_pthreads.c)
endif()

# Set CMAKE_INSTALL_LIBDIR if not defined
include(GNUInstallDirs)

if (NOT ${use_amqp} OR NOT ${use_http})
    set (build_service_client OFF)
    message(STATUS "iothub_service_client build is disabled (AMQP and HTTP support are required)")
endif()

if (NOT ${use_http} AND ${use_prov_client})
    set (build_provisioning_service_client OFF)
    message(STATUS "provisioning_service_client build is disabled (HTTP support is required)")
endif()

if (${use_prov_client_core})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_PROV_MODULE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_PROV_MODULE")

    if ((${build_provisioning_service_client} AND ${use_prov_client}) OR ${run_e2e_tests})
        add_subdirectory(provisioning_service_client)
    endif()

    add_subdirectory(provisioning_client)
endif()

if (${build_service_client})
    add_subdirectory(iothub_service_client)
endif()

if (${run_e2e_tests} OR ${run_longhaul_tests} OR ${nuget_e2e_tests} OR ${run_sfc_tests} OR ${run_unittests})
    add_subdirectory(testtools)
endif()

add_subdirectory(iothub_client)
add_subdirectory(serializer)

if (NOT ${skip_samples})
  add_subdirectory(samples/solutions)
endif()

# add the device_auth sample subdirectory
if (${use_prov_client} AND NOT ${skip_samples})
    add_subdirectory(./provisioning_client/samples)

    if (NOT "${build_python}" STREQUAL "OFF")
        add_subdirectory(../provisioning_device_client provisioning_device_client_python)
    endif()
endif()

if (NOT "${build_python}" STREQUAL "OFF")
    add_subdirectory(../device/iothub_client_python python)
    if (${build_service_client})
        add_subdirectory(../service python_service_client)
    endif()
endif()

if (${use_installed_dependencies})
    # Install azure_iot_sdks
    set(package_location "cmake")

    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${IOT_SDK_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_file("configs/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake"
        COPYONLY
    )

    install(FILES ${parson_h_install_files}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot
    )
    install(TARGETS ${parson_install_libs} EXPORT azure_iot_sdksTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot
    )

    install(EXPORT azure_iot_sdksTargets
        FILE
            "${PROJECT_NAME}Targets.cmake"
        DESTINATION
            ${package_location}
    )

    install(
        FILES
            "configs/${PROJECT_NAME}Config.cmake"
            "configs/${PROJECT_NAME}Functions.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION
            ${package_location}
    )
else()
    install(FILES ${parson_h_install_files}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot)
    install(TARGETS ${parson_install_libs}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

