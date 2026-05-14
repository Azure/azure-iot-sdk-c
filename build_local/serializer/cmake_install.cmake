# Install script for directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/azureiot" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/agenttypesystem.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/codefirst.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/commanddecoder.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/datamarshaller.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/datapublisher.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/dataserializer.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/iotdevice.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/jsondecoder.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/jsonencoder.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/multitree.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/schema.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/schemalib.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/schemaserializer.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/serializer.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/serializer_devicetwin.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/./inc/methodreturn.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/Debug/serializer.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/Release/serializer.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/MinSizeRel/serializer.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/RelWithDebInfo/serializer.lib")
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
