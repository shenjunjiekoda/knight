//===- assert.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines some assertion macros and unreachable code
//  detection.
//
//===------------------------------------------------------------------===//

#undef knight_assert
#undef knight_assert_msg
#undef knight_unreachable

#ifdef NDEBUG

#    define knight_assert(expr) static_cast< void >(0)

#    define knight_assert_msg(expr, msg) static_cast< void >(0)

#    if __has_builtin(__builtin_unreachable)
#        define knight_unreachable(msg) __builtin_unreachable()
#    elif defined(_MSC_VER)
#        define knight_unreachable(msg) __assume(false)
#    else
#        include <cstdlib>
#        define knight_unreachable(msg) abort()
#    endif

#else

#    include <cassert>

#    define knight_assert(expr) assert(expr)
#    define knight_assert_msg(expr, msg) assert((expr) && (msg))
#    define knight_unreachable(msg) assert(false && (msg))

#endif
