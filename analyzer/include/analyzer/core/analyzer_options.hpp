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

namespace knight::analyzer {

constexpr unsigned AnalyzerOptionsAlignment = 8U;
constexpr unsigned DefaultWideningDelay = 1U;
constexpr unsigned DefaultMaxUnrollingIterations = 7U;

/// \brief Options for the knight analyzer
struct alignas(AnalyzerOptionsAlignment) AnalyzerOptions {
    /// \brief Delay count of iterations before widening
    unsigned widening_delay = DefaultWideningDelay;

    /// \brief Maximum number of unrolling iterations
    unsigned max_unrolling_iterations = DefaultMaxUnrollingIterations;

}; // struct AnalyzerOptions

} // namespace knight::analyzer