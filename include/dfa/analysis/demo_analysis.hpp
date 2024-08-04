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

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis_context.hpp"
#include "dfa/domain/demo_dom.hpp"
#include "tooling/context.hpp"

#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class DemoAnalysis : public Analysis< DemoAnalysis,
                                      analyze::BeginFunction,
                                      analyze::PreStmt< clang::ReturnStmt > > {
  public:
    DemoAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::DemoAnalysis;
    }

    void analyze_begin_function(AnalysisContext& C) const {
        llvm::outs() << "DemoAnalysis::analyze_begin_function()\n";
        // TODO: add support to get_state here, now will crash
    }

    void pre_analyze_stmt(const clang::ReturnStmt* S,
                          AnalysisContext& C) const {
        llvm::outs() << "DemoAnalysis::pre_analyze ReturnStmt: \n";
        S->dumpColor();
        llvm::outs() << "\n";

        std::optional< DemoMapDomain* > map_dom_opt =
            C.get_state()->get< DemoMapDomain >();
        if (!map_dom_opt.has_value()) {
            return;
        }

        DemoMapDomain* map_dom = *map_dom_opt;
        llvm::outs() << "state at return: ";
        map_dom->dump(llvm::outs());
        llvm::outs() << "\n";
        auto ret_expr = S->getRetValue();
        if (ret_expr == nullptr) {
            return;
        }

        if (const auto* decl_ref = dyn_cast_or_null< clang::DeclRefExpr >(
                ret_expr->IgnoreParenImpCasts())) {
            if (const clang::ValueDecl* value_decl = decl_ref->getDecl()) {
                const auto& itv = map_dom->get_value(value_decl);

                llvm::outs() << "returning ";
                value_decl->printName(llvm::outs());
                llvm::outs() << "\n";
                llvm::outs() << "itv after return: ";
                itv.dump(llvm::outs());
                llvm::outs() << "\n";
            }
        }
    }

    static void add_dependencies(AnalysisManager& mgr) {
        // add dependencies here
        // mgr.add_domain_dependency< DemoAnalysis, DemoItvDom >();
        mgr.add_domain_dependency< DemoAnalysis, DemoMapDomain >();
    }

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        return mgr.register_analysis< DemoAnalysis >(ctx);
    }

}; // class DemoAnalysis

} // namespace knight::dfa