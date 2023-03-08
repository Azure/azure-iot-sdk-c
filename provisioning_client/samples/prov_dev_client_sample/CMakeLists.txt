#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(prov_dev_client_sample_c_files
    prov_dev_client_sample.c
)

set(prov_dev_client_sample_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(prov_dev_client_sample_c_files ${prov_dev_client_sample_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/adapters)

if (${build_as_dynamic}) 
    set(link_libs iothub_prov_client_dll)   
else()
    set(link_libs ${iothub_client_libs} ${provisioning_libs})
endif()

add_executable(prov_dev_client_sample ${prov_dev_client_sample_c_files} ${prov_dev_client_sample_h_files})

target_link_libraries(prov_dev_client_sample
    ${link_libs} 
    aziotsharedutil
)

if(${use_openssl})
    add_definitions(-DUSE_OPENSSL)
    if (WIN32)
        target_link_libraries(prov_dev_client_sample $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
        file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    endif()
endif()

if (CMAKE_IMPORT_LIBRARY_SUFFIX)
    add_custom_command(TARGET prov_dev_client_sample POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:prov_dev_client_sample> $<TARGET_FILE_DIR:prov_dev_client_sample>
      COMMAND_EXPAND_LISTS
      )
endif()
