#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "OsterLib::lineNoise" for configuration ""
set_property(TARGET OsterLib::lineNoise APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(OsterLib::lineNoise PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/liblineNoise.a"
  )

list(APPEND _cmake_import_check_targets OsterLib::lineNoise )
list(APPEND _cmake_import_check_files_for_OsterLib::lineNoise "${_IMPORT_PREFIX}/lib/liblineNoise.a" )

# Import target "OsterLib::CLInput" for configuration ""
set_property(TARGET OsterLib::CLInput APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(OsterLib::CLInput PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libCLInput.a"
  )

list(APPEND _cmake_import_check_targets OsterLib::CLInput )
list(APPEND _cmake_import_check_files_for_OsterLib::CLInput "${_IMPORT_PREFIX}/lib/libCLInput.a" )

# Import target "OsterLib::boost_functional" for configuration ""
set_property(TARGET OsterLib::boost_functional APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(OsterLib::boost_functional PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libboost_functional.a"
  )

list(APPEND _cmake_import_check_targets OsterLib::boost_functional )
list(APPEND _cmake_import_check_files_for_OsterLib::boost_functional "${_IMPORT_PREFIX}/lib/libboost_functional.a" )

# Import target "OsterLib::types" for configuration ""
set_property(TARGET OsterLib::types APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(OsterLib::types PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libtypes.a"
  )

list(APPEND _cmake_import_check_targets OsterLib::types )
list(APPEND _cmake_import_check_files_for_OsterLib::types "${_IMPORT_PREFIX}/lib/libtypes.a" )

# Import target "OsterLib::utilities" for configuration ""
set_property(TARGET OsterLib::utilities APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(OsterLib::utilities PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libutilities.a"
  )

list(APPEND _cmake_import_check_targets OsterLib::utilities )
list(APPEND _cmake_import_check_files_for_OsterLib::utilities "${_IMPORT_PREFIX}/lib/libutilities.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
