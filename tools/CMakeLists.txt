set(INCLUDE_DIR_LIST_STR "")
foreach(dir ${LLVM_LIBRARY_DIRS})
    set(INCLUDE_DIR_LIST_STR "${INCLUDE_DIR_LIST_STR} -I${dir}/clang/${CLANG_VERSION_MAJOR}/include")
endforeach()

add_definitions(-DINCLUDE_DIRS="${INCLUDE_DIR_LIST_STR}")

add_executable(knight main.cpp)

set_target_properties(knight PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                             "${CMAKE_BINARY_DIR}/bin")

target_link_libraries(knight PRIVATE knight-lib ${CLANG_LIBS} ${LLVM_LIBS})