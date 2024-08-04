//===- symbol.hpp ------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the symbol concept and utility.
//
//===------------------------------------------------------------------===//

#pragma once

#include <concepts>

namespace knight::dfa {

class SymExpr;

template < typename T >
struct is_sym_expr : std::false_type {}; // NOLINT

template < typename T >
struct is_sym_expr< const T* >
    : std::is_base_of< SymExpr, std::remove_cv_t< T > > {};

template < typename T >
concept sym_expr_concept = is_sym_expr< T >::value;

} // namespace knight::dfa