//===- dumpable.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the dumpable concept and utility.
//
//===------------------------------------------------------------------===//

#pragma once

#include "util/log.hpp"

#include <concepts>

namespace knight {

template < typename T >
concept has_dump_os_method = requires(const T& obj, llvm::raw_ostream& os) {
    { obj.dump(os) } -> std::same_as< void >;
} || requires(const std::remove_pointer_t< T >* obj, llvm::raw_ostream& os) {
    { obj->dump(os) } -> std::same_as< void >;
}; // concept has_dump_os_method

template < typename T >
struct DumpableTrait {
    static void dump(llvm::raw_ostream& os, const T& obj) {
        static_assert(has_dump_os_method< T >,
                      "T must have a dump method to use DumpableTrait");
    }
};

template < has_dump_os_method T >
struct DumpableTrait< T > {
    static void dump(llvm::raw_ostream& os, const T& obj) { obj.dump(os); }
};

template < has_dump_os_method T >
struct DumpableTrait< T* > {
    static void dump(llvm::raw_ostream& os, const T* obj) {
        if (obj) {
            obj->dump(os);
        } else {
            os << "nullptr";
        }
    }
};

template < typename T >
struct is_dumpable : std::false_type { // NOLINT(readability-identifier-naming)
};

template < has_dump_os_method T >
struct is_dumpable< T > : std::true_type {};

template < typename T >
constexpr bool is_dumpable_v = // NOLINT(readability-identifier-naming)
    is_dumpable< T >::value;

} // namespace knight