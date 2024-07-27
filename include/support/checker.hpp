//===- checker.hpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the checker concept and utility.
//
//===------------------------------------------------------------------===//

#pragma once

#include <concepts>

namespace knight::dfa {

class CheckerManager;

template < typename CHECKER >
concept checker_has_add_dependencies = requires(CheckerManager& mgr) {
    { CHECKER::add_dependencies(mgr) } -> std::same_as< void >;
};

template < typename CHECKER >
struct is_dependent_checker
    : std::bool_constant< checker_has_add_dependencies< CHECKER > > {
}; // struct is_dependent_checker

} // namespace knight::dfa