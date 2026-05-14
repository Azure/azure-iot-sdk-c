#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "parson::parson" for configuration "MinSizeRel"
set_property(TARGET parson::parson APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(parson::parson PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/parson.lib"
  )

list(APPEND _cmake_import_check_targets parson::parson )
list(APPEND _cmake_import_check_files_for_parson::parson "${_IMPORT_PREFIX}/lib/parson.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
