#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

include(ExternalProject)

if(${dps_hsm_x509})
    if ("${dps_hsm_custom_lib}" STREQUAL "")
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

        IF(WIN32)
            #windows needs this define
            add_definitions(-D_CRT_SECURE_NO_WARNINGS)
            add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
        ENDIF(WIN32)

        add_library(msr_riot ${msr_riot_c_files} ${msr_riot_h_files})
    else()
        set(dps_custom_hsm ON PARENT_SCOPE)
        set(CUSTOM_HSM_LIB "${dps_hsm_custom_lib}" PARENT_SCOPE)
    endif()
else()
    if ("${dps_hsm_custom_lib}" STREQUAL "")
        # Default tpm implementation
        if (${dps_hsm_tpm_simulator})
            set(use_emulator ON CACHE BOOL "enable use_emulator in utpm" FORCE)
        else()
            set(use_emulator OFF CACHE BOOL "disable use_emulator in utpm" FORCE)
        endif()

        add_subdirectory(./utpm)
    else()
    message("Custom Lib value: ${dps_hsm_custom_lib}")
        set(dps_custom_hsm ON PARENT_SCOPE)
        set(CUSTOM_HSM_LIB "${dps_hsm_custom_lib}" PARENT_SCOPE)
    endif()
endif()
