#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "uhttp" for configuration "Debug"
set_property(TARGET uhttp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(uhttp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/uhttp.lib"
  )

list(APPEND _cmake_import_check_targets uhttp )
list(APPEND _cmake_import_check_files_for_uhttp "${_IMPORT_PREFIX}/lib/uhttp.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
