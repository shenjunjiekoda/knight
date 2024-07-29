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
#include "dfa/domain/demo_dom.hpp"
#include "tooling/context.hpp"

#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class DemoAnalysis : public Analysis< DemoAnalysis, analyze::BeginFunction > {
  public:
    DemoAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    static AnalysisKind get_kind() { return AnalysisKind::DemoAnalysis; }

    void analyze_begin_function(AnalysisContext& C) {
        llvm::outs() << "DemoAnalysis::analyze_begin_function()\n";
    }

    static void add_dependencies(AnalysisManager& mgr) {
        // add dependencies here
        mgr.add_domain_dependency< DemoAnalysis, DemoItvDom >();
    }

}; // class DemoAnalysis

} // namespace knight::dfa