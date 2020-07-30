#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

include(ExternalProject)

set(MSR_RIOT_VERSION 1.0.1)

if (${hsm_type_custom})
    set(hsm_type_custom ON PARENT_SCOPE)
    set(CUSTOM_HSM_LIB "${hsm_custom_lib}" PARENT_SCOPE)
else()

    if(${hsm_type_x509})
        set(msr_riot_c_files
            ./RIoT/Simulation/DICE/DiceSha256.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/RiotCrypt.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/aes.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/base64.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/bignum.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/derenc.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/ecdh.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/ecdsa.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/ecp.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/ecp_curves.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/hkdf.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/hmac_drbg.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/md.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/platform_util.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/sha1.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/sha256.c
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/x509bldr.c
        )

        set(msr_riot_h_files
            ./RIoT/Simulation/DICE/DiceSha256.h
            ./RIoT/Simulation/RIoT/Core/RIoT.h
            ./RIoT/Simulation/RIoT/Core/RiotStatus.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/RiotCrypt.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/base64.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/derenc.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/x509bldr.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/aes.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/ecdh.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/ecdsa.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/ecp.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/bignum.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/hkdf.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/hmac_drbg.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/md.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/platform.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/platform_util.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/sha1.h
            ./RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/sha256.h
        )

        if(MSVC)
            set_source_files_properties(./RIoT/Simulation/RIoT/RIoTCrypt/hkdf.c PROPERTIES COMPILE_FLAGS "/wd4100")
        endif()

        add_library(msr_riot ${msr_riot_c_files} ${msr_riot_h_files})

        target_include_directories(msr_riot PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/RIoT/Simulation/DICE>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/RIoT/Simulation/RIoT/Core>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/RIoT/Simulation/RIoT/Core/RIoTCrypt/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls>
        )

        IF(WIN32)
            #windows needs this define
            target_compile_definitions(msr_riot 
                PUBLIC 
                    _CRT_SECURE_NO_WARNINGS
                    GB_MEASURE_MEMORY_FOR_THIS
                    GB_DEBUG_ALLOC
                    WIN32
            )
        ENDIF(WIN32)

        #Set RIoT ECDH Curve & RIOTBUILD
        target_compile_definitions(msr_riot
            PUBLIC
                RIOTBUILD
                RIOTSECP256R1
            )

        set(package_location "cmake")
        install(TARGETS msr_riot EXPORT msr_riotTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/../bin
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot
        )
        install(FILES ${msr_riot_h_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot/msr_riot)

        include(CMakePackageConfigHelpers)

        write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
            VERSION ${MSR_RIOT_VERSION}
            COMPATIBILITY SameMajorVersion
        )

        install(EXPORT msr_riotTargets
            FILE
            "${PROJECT_NAME}Targets.cmake"
            DESTINATION
            ${package_location}
        )

        install(
            FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
            DESTINATION
            ${package_location}
        )
    endif()

    if(${hsm_type_sastoken})
        # Default tpm implementation
        if (${use_tpm_simulator})
            set(use_emulator ON CACHE BOOL "enable use_emulator in utpm" FORCE)
        else()
            set(use_emulator OFF CACHE BOOL "disable use_emulator in utpm" FORCE)
        endif()

        set(original_run_e2e_tests ${run_e2e_tests})
        set(original_run_unittests ${run_unittests})

        set(run_e2e_tests OFF)
        set(run_unittests OFF)

        add_subdirectory(./utpm)

        set(run_e2e_tests ${original_run_e2e_tests})
        set(run_unittests ${original_run_unittests})
    endif()
endif()
