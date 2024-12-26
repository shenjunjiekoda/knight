//===- export.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines some macros required to export symbols when creating
//  a DLL under Windows and to import symbols when using a DLL.
//
// * #define KNIGHT_COMPILE_DLL to compile a DLL under Windows
// * #define KNIGHT_DLL_EXPORT to export symbols when creating the DLL
// * #define KNIGHT_DLL_IMPORT to import symbols when using the DLL
//
//===------------------------------------------------------------------===//

#pragma once

#if defined(_WIN32) && !defined(__GNUC__) && defined(KNIGHT_COMPILE_DLL)
#    if KNIGHT_DLL_EXPORT
#        define KNIGHT_API __declspec(dllexport)
#    else
#        define KNIGHT_API __declspec(dllimport)
#    endif
#else
#    if __GNUC__ >= 4
#        define KNIGHT_API __attribute__((visibility("default")))
#    else
#        define KNIGHT_API
#    endif
#endif

#if defined(WIN32) && defined(KNIGHT_COMPILE_DLL)
#    pragma warning(disable : 4251)
#    pragma warning(disable : 4275)
#endif

// macro taken from https://github.com/nemequ/hedley/blob/master/hedley.h that
// was in public domain at this time
#if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__) || \
    (defined(__INTEL_COMPILER) && __INTEL_COMPILER > 1600) ||       \
    (defined(__ARMCC_VERSION) && __ARMCC_VERSION > 4010000) ||      \
    (defined(__TI_COMPILER_VERSION__) &&                            \
     (__TI_COMPILER_VERSION__ > 8003000 ||                          \
      (__TI_COMPILER_VERSION__ > 7003000 &&                         \
       defined(__TI_GNU_ATTRIBUTE_SUPPORT__))))
#    if defined(__has_attribute)
#        if !defined(KNIGHT_PURE_FUNC) && __has_attribute(pure)
#            define KNIGHT_PURE_FUNC __attribute__((pure))
#        endif
#    endif
#endif
#if !defined(KNIGHT_PURE_FUNC)
#    define KNIGHT_PURE_FUNC
#endif
