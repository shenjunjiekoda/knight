//===- pointer_analysis.hpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the pointer analysis
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/core/analysis/analyses.hpp"
#include "analyzer/core/analysis/analysis_base.hpp"
#include "analyzer/core/domain/domains.hpp"
#include "analyzer/core/domain/pointer.hpp"

namespace knight::analyzer {

class PointerAnalysis
    : public Analysis< PointerAnalysis, analyze::BeginFunction > {
  public:
    explicit PointerAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::PointerAnalysis;
    }

    void analyze_begin_function([[maybe_unused]] AnalysisContext& ctx) const;

    static void add_dependencies(AnalysisManager& mgr) {
        mgr.add_domain_dependency(get_analysis_id(get_kind()), PointerInfoID);
    }

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        return mgr.register_analysis< PointerAnalysis >(ctx);
    }
}; // class PointerAnalysis

} // namespace knight::analyzer