#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "macro_utils_c" for configuration "MinSizeRel"
set_property(TARGET macro_utils_c APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(macro_utils_c PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/macro_utils_c.lib"
  )

list(APPEND _cmake_import_check_targets macro_utils_c )
list(APPEND _cmake_import_check_files_for_macro_utils_c "${_IMPORT_PREFIX}/lib/macro_utils_c.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
