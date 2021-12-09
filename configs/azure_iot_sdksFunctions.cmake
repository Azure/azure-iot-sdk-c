#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

function(getIoTSDKVersion)
    # First find the applicable line in the file
    file (STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/iothub_client/inc/iothub_client_version.h" iotsdkverstr
        REGEX "^[\t ]*#[\t ]*define[\t ]*IOTHUB_SDK_VERSION[\t ]*\"([0-9]+)[\\.]([0-9]+)[\\.]([0-9]+)\"") 
        
    if (!MATCHES)
        message(FATAL_ERROR "Unable to find version in ${CMAKE_SOURCE_DIR}/iothub_client/inc/iothub_client_version.h")
    else(!MATCHES)
        # Parse out the three version identifiers
        set(CMAKE_MATCH_3 "")

        string(REGEX MATCH "^[\t ]*#[\t ]*define[\t ]*IOTHUB_SDK_VERSION[\t ]*\"([0-9]+)[\\.]([0-9]+)[\\.]([0-9]+)\"" temp "${iotsdkverstr}")

        if (NOT "${CMAKE_MATCH_3}" STREQUAL "")
            set (IOT_SDK_VERION_MAJOR "${CMAKE_MATCH_1}" PARENT_SCOPE)
            set (IOT_SDK_VERION_MINOR "${CMAKE_MATCH_2}" PARENT_SCOPE)
            set (IOT_SDK_VERION_FIX "${CMAKE_MATCH_3}" PARENT_SCOPE)
            set (IOT_SDK_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" PARENT_SCOPE)
        else ()
            message(FATAL_ERROR "Unable to find version in ${iotsdkverstr}")
        endif()
    endif(!MATCHES)
endfunction(getIoTSDKVersion)

function(getProvSDKVersion)
    # First find the applicable line in the file   \inc\azure_prov_client
    file (STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/provisioning_client/inc/azure_prov_client/prov_client_const.h" provsdkverstr
        REGEX "^[\t ]*#[\t ]*define[\t ]*PROV_DEVICE_CLIENT_VERSION[\t ]*\"([0-9]+)[\\.]([0-9]+)[\\.]([0-9]+)\"")
        
    if (!MATCHES)
        message(FATAL_ERROR "Unable to find version in ${CMAKE_SOURCE_DIR}/provisioning_client/inc/azure_prov_client/prov_client_const.h")
    else(!MATCHES)
        # Parse out the three version identifiers
        set(CMAKE_MATCH_3 "")

        string(REGEX MATCH "^[\t ]*#[\t ]*define[\t ]*PROV_DEVICE_CLIENT_VERSION[\t ]*\"([0-9]+)[.]([0-9]+)[.]([0-9]+)\"" temp "${provsdkverstr}")

        if (NOT "${CMAKE_MATCH_3}" STREQUAL "")
            set (PROV_SDK_VERSION_MAJOR "${CMAKE_MATCH_1}" PARENT_SCOPE)
            set (PROV_SDK_VERSION_MINOR "${CMAKE_MATCH_2}" PARENT_SCOPE)
            set (PROV_SDK_VERSION_FIX "${CMAKE_MATCH_3}" PARENT_SCOPE)
            set (PROV_SDK_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" PARENT_SCOPE)
        else ()
            message(FATAL_ERROR "Unable to find version in ${provsdkverstr}")
        endif()
    endif(!MATCHES)
endfunction(getProvSDKVersion)

function(linkUAMQP whatExecutableIsBuilding)
    include_directories(${UAMQP_INC_FOLDER})
    
    if(WIN32)
        #windows needs this define
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)

        target_link_libraries(${whatExecutableIsBuilding} uamqp aziotsharedutil ws2_32 secur32)

        if(${use_openssl})
            target_link_libraries(${whatExecutableIsBuilding} $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        
            file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
            file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)

            file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release)
            file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release)
        endif()
    else()
        target_link_libraries(${whatExecutableIsBuilding} uamqp aziotsharedutil ${OPENSSL_LIBRARIES})
    endif()
endfunction(linkUAMQP)

function(includeMqtt)
    include_directories(${MQTT_INC_FOLDER})
endfunction(includeMqtt)

function(linkMqttLibrary whatExecutableIsBuilding)
    includeMqtt()
    target_link_libraries(${whatExecutableIsBuilding} umqtt)
endfunction(linkMqttLibrary)

function(linkUHTTP whatExecutableIsBuilding)
    include_directories(${UHTTP_C_INC_FOLDER})
    target_link_libraries(${whatExecutableIsBuilding} uhttp)
endfunction(linkUHTTP)

function(includeHttp)
endfunction(includeHttp)

function(linkHttp whatExecutableIsBuilding)
    includeHttp()
    if(WIN32)
        target_link_libraries(${whatExecutableIsBuilding} winhttp.lib)
    else()
        target_link_libraries(${whatExecutableIsBuilding} curl)
    endif()
endfunction(linkHttp)

function(linkSharedUtil whatIsBuilding)
    target_link_libraries(${whatIsBuilding} aziotsharedutil)
endfunction(linkSharedUtil)

function(add_unittest_directory test_directory)
    if (${run_unittests})
        add_subdirectory(${test_directory})
    endif()
endfunction()

function(add_e2etest_directory test_directory)
    if (${run_e2e_tests})
        add_subdirectory(${test_directory})
    endif()
endfunction()

function(add_sfctest_directory test_directory)
    if (${run_sfc_tests})
        add_subdirectory(${test_directory})
    endif()
endfunction()

# XCode warns about unused variables and unused static functions,
# both of which are produced by serializer
function(usePermissiveRulesForSdkSamplesAndTests)
    if(XCODE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable  -Wno-unused-function -Wno-overloaded-virtual -Wno-missing-braces" PARENT_SCOPE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable  -Wno-unused-function -Wno-overloaded-virtual -Wno-missing-braces" PARENT_SCOPE)
    endif()
endfunction()

function(add_longhaul_test_directory test_directory)
    if (${run_longhaul_tests})
        add_subdirectory(${test_directory})
    endif()
endfunction()

# For targets which set warning switches as project properties (e.g. XCode)
function(applyXcodeBuildFlagsIfNeeded stbp_target)
    if(XCODE)
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_BOOL_CONVERSION "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_COMMA "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_CONSTANT_CONVERSION "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_EMPTY_BODY "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_ENUM_CONVERSION "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_INFINITE_RECURSION "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_INT_CONVERSION "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_NON_LITERAL_NULL_CONVERSION "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_OBJC_LITERAL_CONVERSION "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_OBJC_ROOT_CLASS "YES_ERROR")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_RANGE_LOOP_ANALYSIS "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_STRICT_PROTOTYPES "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_MOVE "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_UNREACHABLE_CODE "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN__DUPLICATE_METHOD_MATCH "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_64_TO_32_BIT_CONVERSION "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_ABOUT_RETURN_TYPE "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNDECLARED_SELECTOR "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNINITIALIZED_AUTOS "YES_AGGRESSIVE")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNUSED_FUNCTION "YES")
        set_target_properties(${stbp_target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VARIABLE "YES")
    endif()
endfunction()

macro(generate_cppunittest_wrapper whatIsBuilding)
    if (${use_cppunittest} AND WIN32)
      file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${whatIsBuilding}.cxx "#include \"${CMAKE_CURRENT_SOURCE_DIR}/${whatIsBuilding}.c\"")
      set(${whatIsBuilding}_test_files ${CMAKE_CURRENT_BINARY_DIR}/${whatIsBuilding}.cxx)
      #CPP compiler on windows likes to complain about unused local function removed (C4505)
      #C compiler doesn't like to complain about the same thing
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4505")
    else()
set(${whatIsBuilding}_test_files ${whatIsBuilding}.c)
    endif()
endmacro(generate_cppunittest_wrapper)

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
