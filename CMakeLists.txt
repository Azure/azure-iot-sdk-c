# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required (VERSION 3.5)
project(azure_iot_sdks)

########## Enable CMake Features ##########
# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# Set CMAKE_INSTALL_LIBDIR if not defined
include(GNUInstallDirs)
include (CTest)

include("${CMAKE_CURRENT_LIST_DIR}/configs/azure_iot_sdksFunctions.cmake")

# MACOSX_RPATH is enabled
# https://cmake.org/cmake/help/latest/prop_tgt/MACOSX_RPATH.html#prop_tgt:MACOSX_RPATH
if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

getIoTSDKVersion()
message(STATUS "IoT Client SDK Version = ${IOT_SDK_VERSION}")

# Make a global variable to know if we are on linux, windows, or macosx.
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

IF(WIN32)
    # windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

########### CMake Options ###########
# Testing
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF)" OFF)
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(run_longhaul_tests "set run_longhaul_tests to ON to run longhaul tests (default is OFF)[if possible, they are always build]" OFF)
option(run_e2e_openssl_engine_tests "set run_e2e_openssl_engine_tests to ON to run OpenSSL ENGINE tests (default is OFF)[if possible, they are always build]" OFF)
option(use_cppunittest "set use_cppunittest to ON to build CppUnitTest tests on Windows (default is OFF)" OFF)
option(run_sfc_tests "setup the Service Fault tests" OFF)
# Turn ON/OFF samples and upload to blob
option(skip_samples "set skip_samples to ON to skip building samples (default is OFF)[if possible, they are always build]" OFF)
option(dont_use_uploadtoblob "set dont_use_uploadtoblob to ON if the functionality of upload to blob is to be excluded, OFF otherwise. It requires HTTP" OFF)
# Compile options
option(no_logging "disable logging" OFF)
option(use_installed_dependencies "set use_installed_dependencies to ON to use installed packages instead of building dependencies from submodules" OFF)
option(warnings_as_errors  "enable strict compiler warnings-as-errors" ON)
option(build_as_dynamic "build the IoT SDK libaries as dynamic"  OFF)
# Turn on/off IoT features
option(use_prov_client "Enable provisioning client" OFF)
option(use_tpm_simulator "tpm simulator type of hsm used with the provisioning client" OFF)
option(use_edge_modules "Enable support for running modules against Azure IoT Edge" OFF)
option(use_custom_heap "use externally defined heap functions instead of the malloc family" OFF)
option(build_service_client "controls whether the iothub_service_client is built or not" ON)
option(build_provisioning_service_client "controls whether the provisioning_service_client is built or not" ON)
# HSM Type
option(hsm_type_x509 "x509 type of hsm used with the Provisioning client" OFF)
option(hsm_type_sastoken "tpm type of hsm used with the Provisioning client" OFF)
option(hsm_type_symm_key "Symmetric key type of hsm used with the Provisioning client" OFF)
option(hsm_type_custom "hsm type of custom used with the Provisioning client" OFF)
# Transport
option(use_amqp "set use_amqp to ON if amqp is to be used, set to OFF to not use amqp" ON)
option(use_http "set use_http to ON if http is to be used, set to OFF to not use http" ON)
option(use_mqtt "set use_mqtt to ON if mqtt is to be used, set to OFF to not use mqtt" ON)
# TLS Options (OpenSSL further below)
option(use_mbedtls "set use_mbedtls to ON to use mbedtls." OFF)
option(use_bearssl "set use_bearssl to ON to use bearssl." OFF)
option(use_wolfssl "set use_wolfssl to ON to use wolfssl." OFF)
# Compiled-in cert options
option(use_azure_cloud_rsa_cert "set use_azure_cloud_rsa_cert to ON if the Azure Cloud RSA certs are to be used for samples, set to OFF to not use it" OFF)
option(use_azure_cloud_ecc_cert "set use_azure_cloud_ecc_cert to ON if the Azure Cloud ECC certs are to be used for samples, set to OFF to not use it" OFF)
option(use_microsoftazure_de_cert "set use_microsoftazure_de_cert to ON if the MicrosoftAzure DE cert is to be used for samples, set to OFF to not use it" OFF)
option(use_portal_azure_cn_cert "set use_portal_azure_cn_cert to ON if the Portal Azure CN cert is to be used for samples, set to OFF to not use it" OFF)

########## Conditional Options set ##########
if (WIN32 OR MACOSX)
    option(use_openssl "set use_openssl to ON to use OpenSSL." OFF)
else()
    option(use_openssl "set use_openssl to ON to use OpenSSL." ON)
endif()

# OpenSSL samples on Windows need to have a trusted cert set
if ((WIN32 AND ${use_openssl}) OR ${use_wolfssl} OR ${use_mbedtls} OR ${use_bearssl})
    option(use_sample_trusted_cert "Set flag in samples to use SDK's built-in CA as TrustedCerts" ON)
else()
    option(use_sample_trusted_cert "Set flag in samples to use SDK's built-in CA as TrustedCerts" OFF)
endif()

########## String Options ##########
set(hsm_custom_lib "" CACHE STRING "Full path to custom HSM repo library")
set(compileOption_C "" CACHE STRING "passes a string to the command line of the C compiler")
set(compileOption_CXX "" CACHE STRING "passes a string to the command line of the C++ compiler")
set(linkerOption "" CACHE STRING "passes a string to the shared and exe linker options of the C compiler")

# If any compiler has a command line switch called "OFF" then it will need special care
if (NOT "${compileOption_C}" STREQUAL "")
    add_definitions(${compileOption_C})
endif()

if (NOT "${compileOption_CXX}" STREQUAL "")
    add_definitions(${compileOption_CXX})
endif()

if (NOT "${linkerOption}" STREQUAL "")
    message("linkerOption: ${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${linkerOption}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${linkerOption}")
endif()

if (${warnings_as_errors})
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
endif()

if(${use_custom_heap})
    add_definitions(-DGB_USE_CUSTOM_HEAP)
endif()

########## Enable Specific Root Certs ##########
if (${use_azure_cloud_rsa_cert})
    add_definitions(-DUSE_AZURE_CLOUD_RSA_CERT)
endif()

if (${use_azure_cloud_ecc_cert})
    add_definitions(-DUSE_AZURE_CLOUD_ECC_CERT) 
endif()

if (${use_microsoftazure_de_cert})
    add_definitions(-DUSE_MICROSOFTAZURE_DE_CERT)
endif()

if (${use_portal_azure_cn_cert})
    add_definitions(-DUSE_PORTAL_AZURE_CN_CERT)
endif()

########## Set Provisioning Features ON/OFF ##########
if (XCODE AND ${use_prov_client})
    # The TPM module is not available on Mac, and Mac's <string.h> and <unistd.h> files collide as well
    message(FATAL_ERROR "Provisioning client is not supported on Mac.")
endif()

# Default to OFF
set(use_prov_client_core OFF)

# Enable IoT SDK to act as a module for Edge
if(${use_edge_modules})
    set(use_prov_client_core ON)
    set(use_http ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_EDGE_MODULES")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_EDGE_MODULES")
    set(hsm_type_edge_module ON)
endif()

# Set Provisioning Information. This will also setup appropriate HSM
if (${use_prov_client})
    set(use_prov_client_core ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_PROV_MODULE_FULL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_PROV_MODULE_FULL")
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

########## Check for conflicting options ##########
if (NOT ${use_http})
    MESSAGE( "Setting dont_use_uploadtoblob to ON because use_http is OFF")
    set(dont_use_uploadtoblob ON)
    MESSAGE( STATUS "use_http:         " ${use_http} )
    MESSAGE( STATUS "dont_use_uploadtoblob:         " ${dont_use_uploadtoblob} )
endif()

if (NOT ${use_amqp} AND NOT ${use_http} AND NOT ${use_mqtt})
    message(FATAL_ERROR "CMAKE Failure: AMQP, HTTP & MQTT are all disabled, IoT Hub Client must have one protocol enabled.")
endif()

if (${dont_use_uploadtoblob})
    add_definitions(-DDONT_USE_UPLOADTOBLOB)
endif()

if (${no_logging})
    add_definitions(-DNO_LOGGING)
endif()

if (LINUX)
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
        # now all static libraries use PIC flag for Python shared lib
        set(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")
    endif()
endif()

########## Turn off tests for dependencies ##########
set(original_run_e2e_tests ${run_e2e_tests})
set(original_run_unittests ${run_unittests})
set(original_skip_samples ${skip_samples})

set(run_e2e_tests OFF)
set(run_unittests OFF)
set(skip_samples ON)

########## Globally include macro utils and umock ##########
if (IN_OPENWRT)
    add_definitions("$ENV{TARGET_LDFLAGS}" "$ENV{TARGET_CPPFLAGS}" "$ENV{TARGET_CFLAGS}")
    include_directories("$ENV{TOOLCHAIN_DIR}/usr/include" "$ENV{TARGET_LDFLAGS}" "$ENV{TARGET_CPPFLAGS}" "$ENV{TARGET_CFLAGS}")
endif()

########## Add dependencies ##########
if (${original_run_e2e_tests} OR ${original_run_unittests} OR ${run_sfc_tests})
    set(SHARED_UTIL_REAL_TEST_FOLDER ${CMAKE_CURRENT_LIST_DIR}/c-utility/tests/real_test_files CACHE INTERNAL "this is what needs to be included when doing test sources" FORCE)
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/deps)
if(${use_installed_dependencies})
    find_package(parson REQUIRED CONFIG)
endif()

include_directories(${MACRO_UTILS_INC_FOLDER})
include_directories(${UMOCK_C_INC_FOLDER})

if(${use_installed_dependencies})
    if (NOT azure_c_shared_utility_FOUND)
        find_package(azure_c_shared_utility REQUIRED CONFIG)
    endif ()

    if (${use_amqp})
        if (NOT uamqp_FOUND)
            find_package(uamqp REQUIRED CONFIG)
        endif ()
    endif ()

    if (${use_mqtt})
        if (NOT umqtt_FOUND)
            find_package(umqtt REQUIRED CONFIG)
        endif ()
    endif ()

    if (${use_http})
        if (NOT uhttp_FOUND)
            find_package(uhttp REQUIRED CONFIG)
        endif ()
    endif ()
else ()
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/c-utility)

    if (${use_amqp})
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/uamqp)
    endif ()

    if (${use_mqtt})
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/umqtt)
    endif ()

    # For now this must go here and not in deps/CMakeLists.txt for
    # CMake functionality which is in c-utility.
    if (${use_http})
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/deps/uhttp)
    endif ()
endif()

if (${original_run_e2e_tests} OR ${original_run_unittests} OR ${run_sfc_tests})
    # Used for serializer
    add_subdirectory(${SHARED_UTIL_FOLDER}/testtools/sal)
    add_subdirectory(${SHARED_UTIL_FOLDER}/testtools/micromock)
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

message(STATUS "IoT Hub Architecture: ${ARCHITECTURE}")

if (WIN32)
    set(LOCK_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/lock_win32.c)
    set(THREAD_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/threadapi_c11.c)
else()
    set(LOCK_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/lock_pthreads.c)
    set(THREAD_C_FILE ${SHARED_UTIL_ADAPTER_FOLDER}/threadapi_pthreads.c)
endif()

if (NOT ${use_amqp} OR NOT ${use_http})
    set (build_service_client OFF)
    message(STATUS "iothub_service_client build is disabled (AMQP and HTTP support are required)")
endif()

if (NOT ${use_http} AND ${use_prov_client})
    set (build_provisioning_service_client OFF)
    message(STATUS "provisioning_service_client build is disabled (HTTP support is required)")
endif()

########## Add SDK libraries ##########
# Needs to go before the libs for testing CMake variables to resolve
if (${run_e2e_tests} OR ${run_longhaul_tests} OR ${run_sfc_tests} OR ${run_unittests})
    add_subdirectory(testtools)
endif()

if (${build_service_client})
    add_subdirectory(iothub_service_client)
endif()

if (${use_prov_client_core})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_PROV_MODULE")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_PROV_MODULE")

    if ((${build_provisioning_service_client} AND ${use_prov_client}) OR ${run_e2e_tests})
        add_subdirectory(provisioning_service_client)
    endif()

    add_subdirectory(provisioning_client)
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/iothub_client)


# Note this is deprecated
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/serializer)

########## Add SDK samples ##########
if (NOT ${skip_samples})
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/samples/solutions)
endif()

########## Add install components ##########
if (${use_installed_dependencies})
    # Install azure_iot_sdks
    set(package_location "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

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
endif()
