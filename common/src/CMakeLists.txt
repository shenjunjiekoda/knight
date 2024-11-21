file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(knightCommonLib SHARED ${SOURCES})

target_include_directories(knightCommonLib PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})

message(STATUS "Linking with CLANG libraries: ${CLANG_LIBS}")
message(STATUS "Linking with LLVM libraries: ${LLVM_LIBS}")
message(STATUS "Linking with GMP libraries: ${GMP_LIB} ${GMPXX_LIB}")

target_link_libraries(knightCommonLib PUBLIC
    ${CLANG_LIBS}
    ${LLVM_LIBS}
    ${SQLite3_LIBRARIES}
)
