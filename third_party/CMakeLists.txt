if(EXAMPLE_USE_EXTERNAL_JSON)
    find_package(nlohmann_json 3.11.3 REQUIRED)
else()
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/json)
        message(STATUS "thirdparty/json not found. Cloning from https://github.com/nlohmann/json.git...")
        execute_process(
            COMMAND git clone https://github.com/nlohmann/json.git ${CMAKE_SOURCE_DIR}/third_party/json
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE GIT_CLONE_RESULT
        )
        if (NOT GIT_CLONE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to clone nlohmann/json repository.")
        endif()
    endif()

    add_subdirectory(json)
endif()