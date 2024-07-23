# LLVM and Clang setup

if (LLVM_BUILD_DIR)
  message(STATUS "Using LLVM_BUILD_DIR: ${LLVM_BUILD_DIR}")
endif()

set(LLVM_CMAKE_DIR "${LLVM_BUILD_DIR}/lib/cmake/llvm")

find_package(LLVM REQUIRED PATHS ${LLVM_BUILD_DIR} NO_DEFAULT_PATH)
if (NOT LLVM_FOUND)
  message(FATAL_ERROR "LLVM not found.")
endif()

message(STATUS "Found LLVM ${LLVM_VERSION}")
message(STATUS "-> include dir: ${LLVM_INCLUDE_DIRS}")
message(STATUS "-> library dir: ${LLVM_LIBRARY_DIRS}")
message(STATUS "-> bin dir: ${LLVM_TOOLS_BINARY_DIR}")

find_library(LLVM_LIBS NAMES llvm libllvm HINTS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)
if (NOT LLVM_LIBS)
  llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_TARGETS_TO_BUILD}
                                core support option frontendopenmp)
endif()