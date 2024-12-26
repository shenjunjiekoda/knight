file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(knightAnalyzerLib SHARED ${SOURCES})

target_include_directories(knightAnalyzerLib PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)

message(STATUS "Linking with CLANG libraries: ${CLANG_LIBS}")
message(STATUS "Linking with LLVM libraries: ${LLVM_LIBS}")
message(STATUS "Linking with GMP libraries: ${GMP_LIB} ${GMPXX_LIB}")

target_link_libraries(knightAnalyzerLib PUBLIC
    ${CLANG_LIBS}
    ${LLVM_LIBS}
    ${GMP_LIB}
    ${GMPXX_LIB}
    ${SQLite3_LIBRARIES}
    ${COMMON_LIB}
)
