//===- analysis.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the analysis concept and utility.
//
//===------------------------------------------------------------------===//

#pragma once

#include <concepts>

namespace knight::dfa {

class AnalysisManager;

template < typename ANALYSIS >
concept analysis_has_add_dependencies = requires(AnalysisManager& mgr) {
    { ANALYSIS::add_dependencies(mgr) } -> std::same_as< void >;
};

template < typename ANALYSIS >
struct is_dependent_analysis // NOLINT
    : std::bool_constant< analysis_has_add_dependencies< ANALYSIS > > {
}; // struct is_dependent_analysis

} // namespace knight::dfa