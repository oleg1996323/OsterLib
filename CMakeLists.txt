cmake_minimum_required(VERSION 3.16)
project(OsterLib
        VERSION 0.1
        DESCRIPTION "General purpose library with essential utilities"
        LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,$ENV{LD_LIBRARY_PATH}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lzip -static-libstdc++")
add_compile_options("-fPIC" "-O0" "-fconcepts-diagnostics-depth=10")
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost CONFIG 1.82 REQUIRED json)

add_library(bytes INTERFACE)
target_include_directories(bytes INTERFACE 
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

add_library(serial INTERFACE)
target_link_libraries(serial INTERFACE bytes)
target_include_directories(serial INTERFACE 
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

add_library(lineNoise STATIC linenoise/linenoise.c)
target_include_directories(lineNoise PUBLIC 
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/linenoise>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/linenoise>")
add_library(CLInput STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/CLI/CLInavig.cpp)
target_link_libraries(CLInput PRIVATE lineNoise)
target_include_directories(CLInput PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

add_library(compressor INTERFACE)
target_link_libraries(compressor INTERFACE ${ZLIB_LIBRARIES})
target_include_directories(compressor INTERFACE 
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

add_library(images INTERFACE)
target_link_libraries(images INTERFACE PNG::PNG)
target_include_directories(images INTERFACE 
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/images>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/images>")

add_library(boost_functional STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/boost_functional/json.cpp)
target_link_libraries(boost_functional PRIVATE Boost::json)
target_include_directories(boost_functional PUBLIC
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/boost_functional>"
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/boost_functional>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

file(GLOB_RECURSE Network_lib_SOURCES ${CMAKE_SOURCE_DIR}/src/network/**.cpp)

add_library(network SHARED ${Network_lib_SOURCES})
target_link_libraries(network PUBLIC serial bytes)
target_include_directories(network PUBLIC 
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/network>"
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/network>"
                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

add_library(types STATIC src/types/coord.cpp
                src/types/rect.cpp
                src/types/size.cpp
                src/types/time_interval.cpp
                src/types/time_period.cpp)
target_include_directories(types PUBLIC 
                                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/types>"
                                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>"
                                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(types PUBLIC boost_functional)
add_library(utilities STATIC    src/directories.cpp
                                src/floattoieee.cpp
                                src/ibmtofloat.cpp
                                src/int_pow.cpp
                                src/interpolation.cpp)
target_include_directories(utilities PUBLIC
                                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                                "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(TARGETS 
        bytes 
        serial 
        lineNoise 
        CLInput 
        compressor
        boost_functional
        network
        images
        types
        utilities
        EXPORT OsterLib_TARGETS
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT OsterLib_TARGETS
        FILE OsterLib.cmake
        NAMESPACE "OsterLib::"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OsterLib)

export(EXPORT OsterLib_TARGETS
        FILE "${CMAKE_CURRENT_BINARY_DIR}/OsterLib.cmake"
        NAMESPACE "OsterLib::")
write_basic_package_version_file(
    OsterLibConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/OsterLibConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/OsterLibConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/OsterLib")

install(FILES
          "${CMAKE_CURRENT_BINARY_DIR}/OsterLibConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/OsterLibConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/OsterLib"
)

add_subdirectory(tests)
