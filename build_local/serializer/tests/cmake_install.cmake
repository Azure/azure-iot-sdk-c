# Install script for directory: C:/code/s1/azure-iot-sdk-c-pr2720-clean/serializer/tests

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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/agenttypesystem_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/commanddecoder_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/datamarshaller_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/datapublisher_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/dataserializer_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/iotdevice_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/jsondecoder_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/jsonencoder_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/multitree_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/schema_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/schemaserializer_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/methodreturn_ut/cmake_install.cmake")
  include("C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/serializer_int/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/code/s1/azure-iot-sdk-c-pr2720-clean/build_local/serializer/tests/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
