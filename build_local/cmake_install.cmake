# Install script for directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/umock_c" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umock_c.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umock_c_DISABLE_MOCKS.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umock_c_ENABLE_MOCKS.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umock_c_internal.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umock_c_negative_tests.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umock_c_prod.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umock_lock_factory.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umock_lock_factory_default.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umock_lock_if.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umock_log.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umockalloc.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umockautoignoreargs.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umockcall.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umockcallpairs.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umockcallrecorder.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umockstring.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umocktypename.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umocktypes.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umocktypes_bool.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umocktypes_c.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umocktypes_charptr.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umocktypes_stdint.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umocktypes_struct.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umocktypes_wcharptr.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/deps/umock-c/inc/umock_c/umocktypes_windows.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/umock-c/Debug/umock_c.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/umock-c/Release/umock_c.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/umock-c/MinSizeRel/umock_c.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/umock-c/RelWithDebInfo/umock_c.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets.cmake"
         "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/umock_cTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/umock_cTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/umock_cTargets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/umock_cTargets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/umock_cTargets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/umock_cTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/umock_cConfig.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/azure_macro_utils" TYPE FILE FILES
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/azure_macro_utils/macro_utils.h"
    "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/azure_macro_utils/macro_utils_generated.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/c-build-tools/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/azure-macro-utils-c/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/c-logging/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/ctest/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/c-testrunnerswitcher/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/umock-c/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/uamqp/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/umqtt/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/deps/uhttp/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/testtools/sal/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/c-utility/testtools/micromock/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/testtools/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/iothub_service_client/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/provisioning_service_client/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/provisioning_client/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/iothub_client/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/samples/solutions/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
