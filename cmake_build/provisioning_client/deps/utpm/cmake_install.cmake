# Install script for directory: C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/azure_iot_sdks")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/Debug/utpm.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/Release/utpm.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/MinSizeRel/utpm.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/RelWithDebInfo/utpm.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/azureiot/azure_utpm_c" TYPE FILE FILES
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/BaseTypes.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/Capabilities.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/CompilerDependencies.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/GpMacros.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/gbfiledescript.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/Implementation.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/Marshal_fp.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/Memory_fp.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/swap.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/Tpm.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/TPMB.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/TpmBuildSwitches.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/TpmError.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/TpmTypes.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/tpm_codec.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/tpm_comm.h"
    "C:/code/s3/azure-iot-sdk-c/provisioning_client/deps/utpm/./inc/azure_utpm_c/gbfiledescript.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/utpmTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/utpmTargets.cmake"
         "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/utpmTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/utpmTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/utpmTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/utpmTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/utpmTargets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/utpmTargets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/utpmTargets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/utpmTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/utpm/utpmConfigVersion.cmake")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s3/azure-iot-sdk-c/cmake_build/provisioning_client/deps/utpm/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
