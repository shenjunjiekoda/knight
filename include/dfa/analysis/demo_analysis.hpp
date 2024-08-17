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

#define DEBUG_TYPE "DemoAnalysis"

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

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void analyze_begin_function([[maybe_unused]] AnalysisContext& ctx) const {
        LLVM_DEBUG(llvm::outs()
                       << "DemoAnalysis::analyze_begin_function()\nstate:";
                   ctx.get_state()->dump(llvm::outs());
                   llvm::outs() << "\n";);
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void pre_analyze_stmt(const clang::DeclStmt* decl_stmt,
                          AnalysisContext& ctx) const {
        LLVM_DEBUG(llvm::outs() << "DemoAnalysis::pre_analyze DeclStmt: \n";
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

            auto region_opt =
                state->get_region(var_decl, ctx.get_current_stack_frame());
            if (!region_opt) {
                continue;
            }

            const auto* var_region = *region_opt;
            const auto* init_expr = var_decl->getInit()->IgnoreParenImpCasts();

            if (var_region == nullptr) {
                continue;
            }

            clang::Expr::EvalResult eval_int_res;
            bool can_eval_int =
                init_expr->EvaluateAsInt(eval_int_res, ctx.get_ast_context());
            if (!can_eval_int) {
                continue;
            }
            auto int_val = eval_int_res.Val.getInt().getLimitedValue(
                std::numeric_limits< int >::max());
            auto itv = DemoItvDom(static_cast< int >(int_val));
            auto map = state->get_clone< DemoMapDomain >();
            map->set_value(var_region, itv);
            state = state->set< DemoMapDomain >(map);

            LLVM_DEBUG(llvm::outs() << "set domain: "; map->dump(llvm::outs());
                       llvm::outs() << "\n";);
        }
        ctx.set_state(state);
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void pre_analyze_stmt(const clang::ReturnStmt* return_stmt,
                          AnalysisContext& ctx) const {
        LLVM_DEBUG(llvm::outs() << "DemoAnalysis::pre_analyze ReturnStmt: \n";
                   return_stmt->dumpColor();
                   llvm::outs() << "\n";);

        auto map_dom_opt = ctx.get_state()->get_ref< DemoMapDomain >();
        if (!map_dom_opt) {
            return;
        }
        auto& map_dom = *map_dom_opt;

        LLVM_DEBUG(llvm::outs() << "state at return: ";
                   map_dom->dump(llvm::outs());
                   llvm::outs() << "\n";);

        const auto* ret_val_expr = return_stmt->getRetValue();
        if (ret_val_expr == nullptr) {
            return;
        }

        if (const auto* decl_ref = dyn_cast_or_null< clang::DeclRefExpr >(
                ret_val_expr->IgnoreParenImpCasts())) {
            if (const auto* var_decl = llvm::dyn_cast_or_null< clang::VarDecl >(
                    decl_ref->getDecl())) {
                const auto* var_region =
                    ctx.get_region_manager()
                        .get_region(var_decl, ctx.get_current_stack_frame());
                const auto& itv = map_dom->get_value(var_region);

                LLVM_DEBUG(llvm::outs() << "returning ";
                           var_region->dump(llvm::outs());
                           llvm::outs() << "\n";
                           llvm::outs() << "itv after return: ";
                           itv.dump(llvm::outs());
                           llvm::outs() << "\n";);
            }
        }
    }

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