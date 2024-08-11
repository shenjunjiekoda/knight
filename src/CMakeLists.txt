file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(knight-lib SHARED ${SOURCES})

target_include_directories(knight-lib PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_link_libraries(knight-lib PUBLIC util ${CLANG_LIBS} ${LLVM_LIBS})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(knight-lib SHARED ${SOURCES})

target_include_directories(knight-lib PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_link_libraries(knight-lib PUBLIC util ${CLANG_LIBS} ${LLVM_LIBS})

# Ensure the new analysis source file is included
set(ANALYSIS_SOURCES
    src/dfa/analysis/resource_leak_analysis.cpp
    # Add other analysis source files here
)

target_sources(knight-lib PRIVATE ${ANALYSIS_SOURCES})
