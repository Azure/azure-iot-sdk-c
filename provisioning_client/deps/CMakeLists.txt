#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

include(ExternalProject)

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

        add_subdirectory(./utpm EXCLUDE_FROM_ALL)

        set(run_e2e_tests ${original_run_e2e_tests})
        set(run_unittests ${original_run_unittests})
    endif()
endif()
