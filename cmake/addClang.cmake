# Clang setup

set(CLANG_CMAKE_DIR "${LLVM_BUILD_DIR}/lib/cmake/clang")

find_package(Clang REQUIRED PATHS ${LLVM_BUILD_DIR} NO_DEFAULT_PATH)
if (NOT Clang_FOUND)
  message(FATAL_ERROR "Clang not found.")
endif()

if (NOT CLANG_LIBRARY_DIRS)
  foreach(CLANG_INCLUDE_DIR ${CLANG_INCLUDE_DIRS})
    get_filename_component(CLANG_LIBRARY_DIR "${CLANG_INCLUDE_DIR}/../lib" REALPATH)
    list(APPEND CLANG_LIBRARY_DIRS ${CLANG_LIBRARY_DIR})
  endforeach()
endif()

find_library(CLANG_LIBS NAMES clang libclang HINTS ${CLANG_LIBRARY_DIRS} NO_DEFAULT_PATH)
if (NOT CLANG_LIBS)
  set(CLANG_LIBS 
  clangAST
  clangASTMatchers
  clangBasic
  clangDriver
  clangFrontend
  clangRewriteFrontend
  clangSerialization
  clangStaticAnalyzerFrontend
  clangTooling
  clangToolingSyntax)
endif()

if (NOT CLANG_TOOLS_BINARY_DIR)
  set(CLANG_TOOLS_BINARY_DIR "${LLVM_TOOLS_BINARY_DIR}")
endif()

find_file(CLANG_EXECUTABLE NAMES clang PATHS ${CLANG_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
if (NOT CLANG_EXECUTABLE)
  message(FATAL_ERROR "Clang executable not found.")
else()
  message(STATUS "Found Clang executable: ${CLANG_EXECUTABLE}")
  execute_process(COMMAND ${CLANG_EXECUTABLE} --version
                  OUTPUT_VARIABLE CLANG_VERSION_STRING
                  ERROR_QUIET
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "version ([0-9]+\\.[0-9]+\\.[0-9]+)" CLANG_VERSION_MATCH ${CLANG_VERSION_STRING})
  set(CLANG_VERSION ${CMAKE_MATCH_1})
endif()

message(STATUS "Found Clang ${CLANG_VERSION}")
message(STATUS "Found Clang include directory: ${CLANG_INCLUDE_DIRS}")
message(STATUS "Found Clang library directory: ${CLANG_LIBRARY_DIRS}")
message(STATUS "Found Clang binary directory: ${CLANG_TOOLS_BINARY_DIR}")

string(REGEX MATCH "^([0-9]+\\.[0-9]+)" CLANG_VERSION_MAJOR ${CLANG_VERSION})
string(REGEX MATCH "^([0-9]+)" CLANG_VERSION_MAJOR ${CLANG_VERSION_MAJOR})
string(REGEX MATCH "^([0-9]+\\.[0-9]+)" LLVM_VERSION_MAJOR ${LLVM_VERSION})
string(REGEX MATCH "^([0-9]+)" LLVM_VERSION_MAJOR ${LLVM_VERSION_MAJOR})
if (NOT CLANG_VERSION_MAJOR STREQUAL LLVM_VERSION_MAJOR)
  message(FATAL_ERROR "Clang version ${CLANG_VERSION} does not match LLVM version ${LLVM_VERSION}")
endif()

message(STATUS "Using Clang version: ${CLANG_VERSION_MAJOR}")
message(STATUS "Enabled LLVM LIBS: ${LLVM_LIBS}")
message(STATUS "Enabled Clang LIBS: ${CLANG_LIBS}")
