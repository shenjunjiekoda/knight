project(knight-analyzer)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not allowed. Please clean your source tree and try again.")
endif()

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

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "${PROJECT_NAME} (${PACKAGE_VERSION})")
    message(STATUS "- Build type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "- Install prefix: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "- Generator: ${CMAKE_GENERATOR}")
endif()


set(ANALYZER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# LLVM and Clang setup
include(../cmake/addLLVM.cmake)
include(../cmake/addClang.cmake)
# Curl setup
include(../cmake/addCURL.cmake)
include(../cmake/addSqlite3.cmake)

include_directories(
  ${ANALYZER_SRC_DIR}/include 
  ${COMMON_INCLUDE_DIR}
  ${LLVM_INCLUDE_DIRS} 
  ${CLANG_INCLUDE_DIRS}
  ${CURL_INCLUDE_DIRS}
  ${SQLite3_INCLUDE_DIRS})

add_subdirectory(src)
add_subdirectory(tools)

include(../cmake/addCommon.cmake)
include_directories(${COMMON_INCLUDE_DIR})