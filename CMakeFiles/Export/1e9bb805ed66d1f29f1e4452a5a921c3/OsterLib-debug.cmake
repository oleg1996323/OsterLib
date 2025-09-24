#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "OsterLib::lineNoise" for configuration "Debug"
set_property(TARGET OsterLib::lineNoise APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OsterLib::lineNoise PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/liblineNoise.a"
  )

list(APPEND _cmake_import_check_targets OsterLib::lineNoise )
list(APPEND _cmake_import_check_files_for_OsterLib::lineNoise "${_IMPORT_PREFIX}/lib/liblineNoise.a" )

# Import target "OsterLib::CLInput" for configuration "Debug"
set_property(TARGET OsterLib::CLInput APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OsterLib::CLInput PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libCLInput.a"
  )

list(APPEND _cmake_import_check_targets OsterLib::CLInput )
list(APPEND _cmake_import_check_files_for_OsterLib::CLInput "${_IMPORT_PREFIX}/lib/libCLInput.a" )

# Import target "OsterLib::boost_functional" for configuration "Debug"
set_property(TARGET OsterLib::boost_functional APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OsterLib::boost_functional PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libboost_functional.a"
  )

list(APPEND _cmake_import_check_targets OsterLib::boost_functional )
list(APPEND _cmake_import_check_files_for_OsterLib::boost_functional "${_IMPORT_PREFIX}/lib/libboost_functional.a" )

# Import target "OsterLib::types" for configuration "Debug"
set_property(TARGET OsterLib::types APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OsterLib::types PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libtypes.a"
  )

list(APPEND _cmake_import_check_targets OsterLib::types )
list(APPEND _cmake_import_check_files_for_OsterLib::types "${_IMPORT_PREFIX}/lib/libtypes.a" )

# Import target "OsterLib::utilities" for configuration "Debug"
set_property(TARGET OsterLib::utilities APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OsterLib::utilities PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libutilities.a"
  )

list(APPEND _cmake_import_check_targets OsterLib::utilities )
list(APPEND _cmake_import_check_files_for_OsterLib::utilities "${_IMPORT_PREFIX}/lib/libutilities.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
