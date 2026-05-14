#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "uamqp" for configuration "Release"
set_property(TARGET uamqp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(uamqp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/uamqp.lib"
  )

list(APPEND _cmake_import_check_targets uamqp )
list(APPEND _cmake_import_check_files_for_uamqp "${_IMPORT_PREFIX}/lib/uamqp.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
