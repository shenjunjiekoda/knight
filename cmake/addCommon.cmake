if (NOT COMMON_FOUND)
  set(COMMON_INCLUDE_SEARCH_DIRS "")

  # use COMMON_ROOT as a hint
  set(COMMON_ROOT "" CACHE PATH "Path to knight common install directory")

  if (COMMON_ROOT)
    list(APPEND COMMON_INCLUDE_SEARCH_DIRS "${COMMON_ROOT}/include")
  endif()

  # use `knight-config --includedir` as a hint
  find_program(KNIGHT_CONFIG_EXECUTABLE CACHE NAMES knight-config DOC "Path to knight-config binary")

  if (KNIGHT_CONFIG_EXECUTABLE)
    function(run_knight_config FLAG OUTPUT_VAR)
      execute_process(
        COMMAND "${KNIGHT_CONFIG_EXECUTABLE}" "${FLAG}"
        RESULT_VARIABLE HAD_ERROR
        OUTPUT_VARIABLE ${OUTPUT_VAR}
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
      )
      if (HAD_ERROR)
        message(FATAL_ERROR "knight-config failed with status: ${HAD_ERROR}")
      endif()
      set(${OUTPUT_VAR} "${${OUTPUT_VAR}}" PARENT_SCOPE)
    endfunction()

    run_knight_config("--includedir" KNIGHT_CONFIG_INCLUDE_DIR)

    list(APPEND COMMON_INCLUDE_SEARCH_DIRS "${KNIGHT_CONFIG_INCLUDE_DIR}")
  endif()

  find_path(COMMON_INCLUDE_DIR
    NAMES knight/common/util/assert.hpp
    HINTS ${COMMON_INCLUDE_SEARCH_DIRS}
    DOC "Path to knight common include directory"
  )

  find_library(COMMON_LIB
    NAMES knightCommonLib
    HINTS ${FRONTEND_LLVM_LIB_SEARCH_DIRS}
    DOC "Path to ikos llvm-to-ar library"
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Common
    REQUIRED_VARS
      COMMON_INCLUDE_DIR
      COMMON_LIB
    FAIL_MESSAGE
      "Could NOT find knight common. Please provide -DCOMMON_ROOT=/path/to/common")
endif()
