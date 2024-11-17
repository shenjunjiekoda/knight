# sqlite3 setup
find_package(SQLite3 REQUIRED)

if(NOT SQLITE3_FOUND)
    set(SQLITE3_DIR "" CACHE PATH "sqlite3 directory")

    find_path(SQLite3_INCLUDE_DIRS
        NAMES sqlite3.h
        HINTS "${SQLITE3_DIR}/include"
        DOC "sqlite3 include directory"
    )

    find_library(SQLite3_LIBRARIES
        NAMES sqlite3
        HINTS "${SQLITE3_DIR}/lib"
        DOC "sqlite3 library"
    )

    if(SQLite3_INCLUDE_DIRS AND SQLITE3_LIBRARIES)
        file(WRITE "${PROJECT_BINARY_DIR}/FindSQLite3Version.c" "
      #include <assert.h>
      #include <stdio.h>
      #include <string.h>
      #include <sqlite3.h>

      int main() {
      // The following assertion does not always hold on macs, due to a bug in
      // the sqlite3 setup shipped on Mac. So, we only check if the OS is not
      // Apple.
      #ifndef __APPLE__
        assert(strcmp(SQLITE_VERSION, sqlite3_libversion()) == 0);
      #endif
        printf(\"%s\", sqlite3_libversion());
        return 0;
      }
    ")

        try_run(
            RUN_RESULT
            COMPILE_RESULT
            "${PROJECT_BINARY_DIR}"
            "${PROJECT_BINARY_DIR}/FindSQLite3Version.c"
            CMAKE_FLAGS
            "-DINCLUDE_DIRECTORIES:STRING=${SQLite3_INCLUDE_DIRS}"
            "-DLINK_LIBRARIES:STRING=${SQLite3_LIBRARIES}"
            COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
            RUN_OUTPUT_VARIABLE SQLITE3_VERSION
        )

        if(NOT COMPILE_RESULT)
            message(FATAL_ERROR "error when trying to compile a program with SQLite3:\n${COMPILE_OUTPUT}")
        endif()

        if(RUN_RESULT)
            message(FATAL_ERROR "error when running a program linked with SQLite3:\n${SQLite3_VERSION}")
        endif()
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(SQLite3
        REQUIRED_VARS
        SQLite3_INCLUDE_DIRS
        SQLite3_LIBRARIES
        VERSION_VAR
        SQLite3_VERSION
        FAIL_MESSAGE
        "Could NOT find SQLite3. Please provide -DSQLITE3_DIR=/path/to/sqlite3")
endif()

message(STATUS "SQLite3 found: ${SQLITE3_FOUND} (${SQLite3_VERSION})")
message(STATUS "SQLite3 include dir: ${SQLite3_INCLUDE_DIRS}")
message(STATUS "SQLite3 library: ${SQLite3_LIBRARIES}")