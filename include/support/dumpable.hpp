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

#include <llvm/Support/raw_ostream.h>

#include <concepts>

namespace knight {

template < typename T >
concept dumpable = requires(const T& obj, llvm::raw_ostream& os) {
    { obj.dump(os) } -> std::same_as< void >;
};

template < dumpable T > struct DumplableTrait {
    static void dump(llvm::raw_ostream& os, const T& obj) { obj.dump(os); }
};

template < typename T >
struct is_dumpable : std::bool_constant< dumpable< T > > {};

} // namespace knight