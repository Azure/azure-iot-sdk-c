# Install script for directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2

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
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/c-logging/v2/Debug/c_logging_v2.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/c-logging/v2/Release/c_logging_v2.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/c-logging/v2/MinSizeRel/c_logging_v2.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/c-logging/v2/RelWithDebInfo/c_logging_v2.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/c_logging/v2/c_logging" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/logger.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/logger_v1_v2.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_context.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_context_property_type.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_context_property_type_if.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_context_property_basic_types.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_context_property_bool_type.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_context_property_to_string.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_context_property_type_ascii_char_ptr.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_context_property_type_struct.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_context_property_value_pair.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_context_property_type_wchar_t_ptr.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_errno.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_internal_error.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_level.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_sink_if.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_sink_console.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_sink_callback.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/logging_stacktrace.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/format_message_no_newline.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_sink_etw.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_lasterror.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/log_hresult.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/c-utility/deps/c-logging/v2/./inc/c_logging/get_thread_stack.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/c-logging/v2/tests/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/deps/c-logging/v2/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
