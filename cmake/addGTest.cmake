find_package(GTest REQUIRED)

if(GTEST_FOUND)
    set(GTEST_LIBS GTest::GTest GTest::Main)
else()
    message(FATAL_ERROR "Could not find Google Test library")
endif()

function(add_gtest target_name sources extra_libs)
    add_executable(${target_name} ${sources})
    target_link_libraries(${target_name} PRIVATE ${extra_libs} ${GTEST_LIBS})
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()
