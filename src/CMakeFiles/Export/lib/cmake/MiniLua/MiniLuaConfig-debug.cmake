#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MiniLua" for configuration "Debug"
set_property(TARGET MiniLua APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MiniLua PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libMiniLua.so"
  IMPORTED_SONAME_DEBUG "libMiniLua.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS MiniLua )
list(APPEND _IMPORT_CHECK_FILES_FOR_MiniLua "${_IMPORT_PREFIX}/lib/libMiniLua.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
