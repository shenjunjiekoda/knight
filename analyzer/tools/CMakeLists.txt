set(INCLUDE_DIR_LIST_STR "")

foreach(dir ${LLVM_LIBRARY_DIRS})
    set(INCLUDE_DIR_LIST_STR "${INCLUDE_DIR_LIST_STR} -I${dir}/clang/${CLANG_VERSION_MAJOR}/include")
endforeach()

add_definitions(-DINCLUDE_DIRS="${INCLUDE_DIR_LIST_STR}")

add_executable(knight-analyzer main.cpp)

if(NOT LLVM_ENABLE_RTTI)
  if(MSVC)
    target_compile_options(knight-analyzer PRIVATE /GR-)
  else()
    target_compile_options(knight-analyzer PRIVATE -fno-rtti)
  endif()
endif()

# CMake sets MSVC for both MSVC and Clang(Windows)
if (MSVC)
  # Common MSVC/Clang(Windows) options
  target_compile_options(knight-analyzer PRIVATE
    /nologo
    /EHsc
    /D_CRT_SECURE_NO_WARNINGS # don't try to use MSVC std replacements
    /W3 # roughly -Wall
    /wd4996 # ignore deprecated declaration
    /wd4267 # ignores warning C4267
            # (conversion from 'size_t' to 'type'),
            # roughly -Wno-sign-compare
    /wd4800
    /wd4068 # Disable unknown pragma warning
    /std:c++17
    /Zc:__cplusplus
    $<$<CONFIG:Debug>:/FS>
  )
  # relink system libs
  target_link_libraries(knight-analyzer PRIVATE Mincore.lib)
endif()

set_target_properties(knight-analyzer PROPERTIES RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin")

target_link_libraries(knight-analyzer PUBLIC
    knightAnalyzerLib
)

