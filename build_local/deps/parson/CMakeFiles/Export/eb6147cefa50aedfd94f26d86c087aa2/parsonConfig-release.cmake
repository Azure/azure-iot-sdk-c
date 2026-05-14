#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "parson::parson" for configuration "Release"
set_property(TARGET parson::parson APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(parson::parson PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/parson.lib"
  )

list(APPEND _cmake_import_check_targets parson::parson )
list(APPEND _cmake_import_check_files_for_parson::parson "${_IMPORT_PREFIX}/lib/parson.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
