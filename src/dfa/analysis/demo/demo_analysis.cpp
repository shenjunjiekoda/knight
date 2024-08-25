//===- demo_analysis.cpp ----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements some demo analysis for testing purpose.
//
//===------------------------------------------------------------------===//

#include "dfa/analysis/demo/demo_analysis.hpp"

#define DEBUG_TYPE "DemoAnalysis"

namespace knight::dfa {

void DemoAnalysis::analyze_begin_function(
    [[maybe_unused]] AnalysisContext& ctx) const {
    knight_log_nl(llvm::outs()
                      << "DemoAnalysis::analyze_begin_function()\nstate:";
                  ctx.get_state()->dump(llvm::outs());
                  llvm::outs() << "\n";);
}

void DemoAnalysis::pre_analyze_stmt(const clang::DeclStmt* decl_stmt,
                                    AnalysisContext& ctx) const {
    knight_log_nl(llvm::outs() << "DemoAnalysis::pre_analyze DeclStmt: \n";
                  decl_stmt->dumpColor();
                  llvm::outs() << "\n";);

    auto state = ctx.get_state();
    for (const auto* decl : decl_stmt->decls()) {
        const auto* var_decl = dyn_cast_or_null< clang::VarDecl >(decl);
        if (var_decl == nullptr || var_decl->getInit() == nullptr) {
            continue;
        }

        knight_log_nl(llvm::outs() << "var decl: "; var_decl->dumpColor();
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

        knight_log_nl(llvm::outs() << "set domain: "; map->dump(llvm::outs());
                      llvm::outs() << "\n";);
    }
    ctx.set_state(state);
}

void DemoAnalysis::pre_analyze_stmt(const clang::ReturnStmt* return_stmt,
                                    AnalysisContext& ctx) const {
    knight_log_nl(llvm::outs() << "DemoAnalysis::pre_analyze ReturnStmt: \n";
                  return_stmt->dumpColor();
                  llvm::outs() << "\n";);

    auto map_dom_opt = ctx.get_state()->get_ref< DemoMapDomain >();
    if (!map_dom_opt) {
        return;
    }
    auto& map_dom = *map_dom_opt;

    knight_log_nl(llvm::outs() << "state at return: ";
                  map_dom->dump(llvm::outs());
                  llvm::outs() << "\n";);

    const auto* ret_val_expr = return_stmt->getRetValue();
    if (ret_val_expr == nullptr) {
        return;
    }

    if (const auto* decl_ref = dyn_cast_or_null< clang::DeclRefExpr >(
            ret_val_expr->IgnoreParenImpCasts())) {
        if (const auto* var_decl =
                llvm::dyn_cast_or_null< clang::VarDecl >(decl_ref->getDecl())) {
            const auto* var_region =
                ctx.get_region_manager()
                    .get_region(var_decl, ctx.get_current_stack_frame());
            const auto& itv = map_dom->get_value(var_region);

            knight_log_nl(llvm::outs() << "returning ";
                          var_region->dump(llvm::outs());
                          llvm::outs() << "\n";
                          llvm::outs() << "itv after return: ";
                          itv.dump(llvm::outs());
                          llvm::outs() << "\n";);
        }
    }
}

} // namespace knight::dfa