//===- analyzer_options.hpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the options for the knight analyzer.
//
//===------------------------------------------------------------------===//

#pragma once

namespace knight::dfa {

constexpr unsigned AnalyzerOptionsAlignment = 16U;

/// \brief Options for the knight analyzer
struct alignas(AnalyzerOptionsAlignment) AnalyzerOptions {
    /// \brief Delay count of iterations before widening
    int widening_delay = 1;

    /// \brief Maximum number of widening iterations
    int max_widening_iterations = 5;

    /// \brief Maximum number of narrowing iterations
    int max_narrowing_iterations = 5;

    /// \brief If true, do widening and narrowing with threshold
    bool analyze_with_threshold = false;

}; // struct AnalyzerOptions

} // namespace knight::dfa