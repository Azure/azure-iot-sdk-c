# Install script for directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_service_client

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/azure_iot_sdks")
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/iothub_service_client" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_service_client/./inc/iothub_deviceconfiguration.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_service_client/./inc/iothub_devicemethod.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_service_client/./inc/iothub_devicetwin.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_service_client/./inc/iothub_messaging.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_service_client/./inc/iothub_messaging_ll.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_service_client/./inc/iothub_registrymanager.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_service_client/./inc/iothub_sc_version.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_service_client/./inc/iothub_service_client_auth.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/iothub_service_client/../iothub_client/inc/iothub_message.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_service_client/Debug/iothub_service_client.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_service_client/Release/iothub_service_client.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_service_client/MinSizeRel/iothub_service_client.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_service_client/RelWithDebInfo/iothub_service_client.lib")
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_service_client/tests/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_validation/iothub_service_client/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
