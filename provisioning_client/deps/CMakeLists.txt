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
            ./RIoT/Emulator/DICE/DiceSha256.c
            ./RIoT/Emulator/RIoT/RIoTCrypt/RiotAes128.c
            ./RIoT/Emulator/RIoT/RIoTCrypt/RiotAesTables.c
            ./RIoT/Emulator/RIoT/RIoTCrypt/RiotCrypt.c
            ./RIoT/Emulator/RIoT/RIoTCrypt/RiotDerEnc.c
            ./RIoT/Emulator/RIoT/RIoTCrypt/RiotEcc.c
            ./RIoT/Emulator/RIoT/RIoTCrypt/RiotHmac.c
            ./RIoT/Emulator/RIoT/RIoTCrypt/RiotKdf.c
            ./RIoT/Emulator/RIoT/RIoTCrypt/RiotSha256.c
            ./RIoT/Emulator/RIoT/RIoTCrypt/RiotX509Bldr.c
            ./RIoT/Emulator/RIoT/RIoTCrypt/RiotBase64.c
        )

        set(msr_riot_h_files
            ./RIoT/Emulator/DICE/DiceSha256.h
            ./RIoT/Emulator/RIoT/RIoT.h
            ./RIoT/Emulator/RIoT/RiotStatus.h
            ./RIoT/Emulator/RIoT/RIoTCrypt/include/RiotAes128.h
            ./RIoT/Emulator/RIoT/RIoTCrypt/include/RiotCrypt.h
            ./RIoT/Emulator/RIoT/RIoTCrypt/include/RiotDerEnc.h
            ./RIoT/Emulator/RIoT/RIoTCrypt/include/RiotEcc.h
            ./RIoT/Emulator/RIoT/RIoTCrypt/include/RiotHmac.h
            ./RIoT/Emulator/RIoT/RIoTCrypt/include/RiotKdf.h
            ./RIoT/Emulator/RIoT/RIoTCrypt/include/RiotSha256.h
            ./RIoT/Emulator/RIoT/RIoTCrypt/include/RiotTarget.h
            ./RIoT/Emulator/RIoT/RIoTCrypt/include/RiotX509Bldr.h
            ./RIoT/Emulator/RIoT/RIoTCrypt/include/RiotBase64.h
        )

        include_directories(./RIoT/Emulator/DICE)
        include_directories(./RIoT/Emulator/RIoT)
        include_directories(./RIoT/Emulator/RIoT/RIoTCrypt/include)

        if(MSVC)
            set_source_files_properties(./RIoT/Emulator/RIoT/RIoTCrypt/RiotKdf.c PROPERTIES COMPILE_FLAGS "/wd4100")
        endif()

        IF(WIN32)
            #windows needs this define
            add_definitions(-D_CRT_SECURE_NO_WARNINGS)
            add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
            add_definitions(-DWIN32)
        ENDIF(WIN32)

        add_library(msr_riot ${msr_riot_c_files} ${msr_riot_h_files})
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
