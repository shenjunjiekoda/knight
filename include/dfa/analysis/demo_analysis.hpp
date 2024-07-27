//===- demo_analysis.hpp ----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines some demo analysis for testing purpose.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis_context.hpp"
#include "tooling/context.hpp"

#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class DemoAnalysis : public Analysis< analyze::BeginFunction > {
  public:
    DemoAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    AnalysisKind get_kind() const override {
        return AnalysisKind::DemoAnalysis;
    }

    void analyze_begin_function(AnalysisContext& C) {
        llvm::outs() << "DemoAnalysis::analyze_begin_function()\n";
    }

    // static

}; // class DemoAnalysis

} // namespace knight::dfa