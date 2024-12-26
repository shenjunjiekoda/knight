file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp)
add_definitions(-DLLVM_LIBRARY_DIRS=\"${LLVM_LIBRARY_DIRS}\")
add_definitions(-DLLVM_PACKAGE_VERSION=\"${LLVM_PACKAGE_VERSION}\")
add_library(knightCommonLib SHARED ${SOURCES})

if(SQLITE_HAS_ENCRYPTION)
    target_compile_definitions(knightCommonLib PUBLIC SQLITE_HAS_ENCRYPTION)
endif()

target_include_directories(knightCommonLib PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})

message(STATUS "Linking with CLANG libraries: ${CLANG_LIBS}")
message(STATUS "Linking with LLVM libraries: ${LLVM_LIBS}")

target_link_libraries(knightCommonLib PUBLIC
    ${CLANG_LIBS}
    ${LLVM_LIBS}
    ${SQLite3_LIBRARIES}
)
