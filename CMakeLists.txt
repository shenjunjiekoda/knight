
cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

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

# LLVM and Clang setup
include(cmake/addLLVM.cmake)
include(cmake/addClang.cmake)

include_directories(${SRC_DIR}/include ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})

add_subdirectory(src)
add_subdirectory(tools)