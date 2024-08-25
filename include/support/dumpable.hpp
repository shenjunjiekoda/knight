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
concept dumpable = requires(const T& obj, llvm::raw_ostream& os) {
    { obj.dump(os) } -> std::same_as< void >;
} || requires(const std::remove_pointer_t< T >* obj, llvm::raw_ostream& os) {
    { obj->dump(os) } -> std::same_as< void >;
}; // concept dumpable

template < dumpable T >
struct DumpableTrait {
    static void dump(llvm::raw_ostream& os, const T& obj) {
        if constexpr (std::is_pointer_v< T >) {
            if (obj) {
                obj->dump(os);
            } else {
                os << "nullptr";
            }
        } else {
            obj.dump(os);
        }
    }
};
// struct DumpableTrait< T* >

template < typename T >
struct is_dumpable : std::bool_constant< dumpable< T > > { // NOLINT
}; // struct is_dumpable

} // namespace knight