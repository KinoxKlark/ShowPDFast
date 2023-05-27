cmake_minimum_required(VERSION 3.10)

project(ShowPDFast VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option for building static or shared library
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

# vspkg toolchain options
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE})

# Set PoDoFo build options
set(PODOFO_BUILD_LIB_ONLY TRUE)
set(PODOFO_BUILD_STATIC TRUE)
add_subdirectory(external/podofo)
include_directories(external/podofo/src)

# Include source directory
add_subdirectory(src)

# Include demo directory
add_subdirectory(demo)

