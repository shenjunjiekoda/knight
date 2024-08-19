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
#include "dfa/region/region.hpp"
#include "tooling/context.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class DemoAnalysis : public Analysis< DemoAnalysis,
                                      analyze::BeginFunction,
                                      analyze::PreStmt< clang::ReturnStmt >,
                                      analyze::PreStmt< clang::DeclStmt > > {
  public:
    explicit DemoAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::DemoAnalysis;
    }

    void analyze_begin_function([[maybe_unused]] AnalysisContext& ctx) const;

    void pre_analyze_stmt(const clang::DeclStmt* decl_stmt,
                          AnalysisContext& ctx) const;

    void pre_analyze_stmt(const clang::ReturnStmt* return_stmt,
                          AnalysisContext& ctx) const;

    static void add_dependencies(AnalysisManager& mgr) {
        // add dependencies here
        mgr.add_domain_dependency< DemoAnalysis, DemoMapDomain >();
    }

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        return mgr.register_analysis< DemoAnalysis >(ctx);
    }

}; // class DemoAnalysis

} // namespace knight::dfa