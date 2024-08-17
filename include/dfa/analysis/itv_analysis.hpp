//===- intervcal_analysis.hpp -----------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines an interval analysis
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis_context.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/domain/demo_dom.hpp"
#include "dfa/domain/interval_dom.hpp"
#include "dfa/region/region.hpp"
#include "tooling/context.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "itv_analysis"

namespace knight::dfa {

class ItvAnalysis : public Analysis< ItvAnalysis,
                                     analyze::BeginFunction,
                                     analyze::PreStmt< clang::ReturnStmt >,
                                     analyze::PreStmt< clang::DeclStmt > > {
  public:
    explicit ItvAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::ItvAnalysis;
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void analyze_begin_function([[maybe_unused]] AnalysisContext& ctx) const {
        LLVM_DEBUG(llvm::outs()
                       << "ItvAnalysis::analyze_begin_function()\nstate:";
                   ctx.get_state()->dump(llvm::outs());
                   llvm::outs() << "\n";);
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void pre_analyze_stmt(const clang::DeclStmt* decl_stmt,
                          AnalysisContext& ctx) const {
        LLVM_DEBUG(llvm::outs() << "ItvAnalysis::pre_analyze DeclStmt: \n";
                   decl_stmt->dumpColor();
                   llvm::outs() << "\n";);

        auto state = ctx.get_state();
        for (const auto* decl : decl_stmt->decls()) {
            const auto* var_decl = dyn_cast_or_null< clang::VarDecl >(decl);
            if (var_decl == nullptr || var_decl->getInit() == nullptr) {
                continue;
            }

            LLVM_DEBUG(llvm::outs() << "var decl: "; var_decl->dumpColor();
                       llvm::outs() << "\n";);

            auto zvar_opt =
                state->try_get_zvariable(var_decl,
                                         ctx.get_current_stack_frame());
            if (!zvar_opt) {
                continue;
            }

            ZVariable zvar = *zvar_opt;
            const auto* init_expr = var_decl->getInit()->IgnoreParenImpCasts();
        }
        ctx.set_state(state);
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void pre_analyze_stmt(const clang::ReturnStmt* return_stmt,
                          AnalysisContext& ctx) const {
        LLVM_DEBUG(llvm::outs() << "ItvAnalysis::pre_analyze ReturnStmt: \n";
                   return_stmt->dumpColor();
                   llvm::outs() << "\n";);
    }

    static void add_dependencies(AnalysisManager& mgr) {
        // add dependencies here
        mgr.add_domain_dependency< ItvAnalysis, ZIntervalDom >();
    }

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        return mgr.register_analysis< ItvAnalysis >(ctx);
    }

}; // class ItvAnalysis

} // namespace knight::dfa