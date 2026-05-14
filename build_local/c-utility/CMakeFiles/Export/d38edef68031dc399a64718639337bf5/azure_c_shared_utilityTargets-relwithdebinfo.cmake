#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "aziotsharedutil" for configuration "RelWithDebInfo"
set_property(TARGET aziotsharedutil APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(aziotsharedutil PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/aziotsharedutil.lib"
  )

list(APPEND _cmake_import_check_targets aziotsharedutil )
list(APPEND _cmake_import_check_files_for_aziotsharedutil "${_IMPORT_PREFIX}/lib/aziotsharedutil.lib" )

# Import target "c_logging_v2" for configuration "RelWithDebInfo"
set_property(TARGET c_logging_v2 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(c_logging_v2 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/c_logging_v2.lib"
  )

list(APPEND _cmake_import_check_targets c_logging_v2 )
list(APPEND _cmake_import_check_files_for_c_logging_v2 "${_IMPORT_PREFIX}/lib/c_logging_v2.lib" )

# Import target "c_logging_v2_core" for configuration "RelWithDebInfo"
set_property(TARGET c_logging_v2_core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(c_logging_v2_core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/c_logging_v2_core.lib"
  )

list(APPEND _cmake_import_check_targets c_logging_v2_core )
list(APPEND _cmake_import_check_files_for_c_logging_v2_core "${_IMPORT_PREFIX}/lib/c_logging_v2_core.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
