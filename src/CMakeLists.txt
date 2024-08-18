file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(knight-lib SHARED ${SOURCES})

target_include_directories(knight-lib PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})

target_link_libraries(knight-lib PUBLIC util
    ${CLANG_LIBS}
    ${LLVM_LIBS}
    ${GMP_LIB}
    ${GMPXX_LIB}
)
