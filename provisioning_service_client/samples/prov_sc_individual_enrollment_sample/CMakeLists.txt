#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for prov_sc_individual_enrollment_sample

compileAsC99()

set(prov_sc_individual_enrollment_sample_c_files
    prov_sc_individual_enrollment_sample.c
    ../../../certs/certs.c
)

set (prov_sc_individual_enrollment_sample_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

include_directories(${PROVISIONING_SERVICE_CLIENT_INC_FOLDER})

add_executable(prov_sc_individual_enrollment_sample ${prov_sc_individual_enrollment_sample_c_files} ${prov_sc_individual_enrollment_sample_h_files})

target_link_libraries(prov_sc_individual_enrollment_sample
    provisioning_service_client)

if(${use_openssl})
    add_definitions(-DUSE_OPENSSL)
    if (WIN32)
        target_link_libraries(prov_sc_individual_enrollment_sample $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
        file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    endif()
endif()


linkSharedUtil(prov_sc_individual_enrollment_sample)
linkUHTTP(prov_sc_individual_enrollment_sample)