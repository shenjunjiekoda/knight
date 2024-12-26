
cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)

project(knight)

set(PACKAGE_VERSION "1.0.0")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
    "Release"
    CACHE
    STRING
    "Build mode ('DebugSan' or 'Debug' or 'Release', default is 'Debug')"
    FORCE)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
    "${CMAKE_SOURCE_DIR}/install"
    CACHE PATH "cmake install directory" FORCE)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

message(STATUS "${PROJECT_NAME} (${PACKAGE_VERSION})")
message(STATUS "- Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "- Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "- Generator: ${CMAKE_GENERATOR}")

set(SRC_DIR ${CMAKE_SOURCE_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

option(LINK_LLVM_DYLIB "Link with libLLVM dynamic library" OFF)
option(LINK_CLANG_DYLIB "Link with libclang-cpp dynamic library" OFF)
option(EXAMPLE_USE_EXTERNAL_JSON "Use an external JSON library" OFF)
option(SQLITE_HAS_ENCRYPTION "Enable SQLite encryption support" OFF)
option(BUILD_TESTS "Build and run tests." OFF)

if(WIN32)
  message(STATUS "Build shared libraries (DLLs).")
  add_definitions(-DKNIGHT_DLL_EXPORT=1)
  add_definitions(-DKNIGHT_DLL_IMPORT=1)
endif()

message(STATUS "Including common")
add_subdirectory(common)

set(COMMON_FOUND TRUE)
set(COMMON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common/include")
set(COMMON_LIB knightCommonLib)

# Add cg
message(STATUS "Including cg")
add_subdirectory(cg)

# Add analyzer
message(STATUS "Including analyzer")
add_subdirectory(analyzer)