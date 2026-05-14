# Install script for directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c

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

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/umock-c/Debug/umock_c.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/umock-c/Release/umock_c.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/umock-c/MinSizeRel/umock_c.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/umock-c/RelWithDebInfo/umock_c.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/umock_c" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umock_c.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umock_c_internal.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umock_c_negative_tests.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umock_c_prod.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umock_lock_factory.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umock_lock_factory_default.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umock_lock_if.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umock_log.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umockalloc.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umockautoignoreargs.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umockcall.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umockcallpairs.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umockcallrecorder.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umockstring.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umocktypename.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umocktypes.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umocktypes_bool.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umocktypes_c.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umocktypes_stdint.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umocktypes_charptr.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umocktypes_struct.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umocktypes_wcharptr.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/umock-c/./inc/umock_c/umocktypes_windows.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/umock-c/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
