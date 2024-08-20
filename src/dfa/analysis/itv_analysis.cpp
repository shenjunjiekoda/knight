
//===- intervcal_analysis.cpp -----------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements an interval analysis
//
//===------------------------------------------------------------------===//

#include "dfa/analysis/itv_analysis.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/domain/interval_dom.hpp"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "ItvAnalysis"

namespace knight::dfa {

void ItvAnalysis::analyze_begin_function(
    [[maybe_unused]] AnalysisContext& ctx) const {
    LLVM_DEBUG(llvm::outs() << "ItvAnalysis::analyze_begin_function()\nstate:";
               ctx.get_state()->dump(llvm::outs());
               llvm::outs() << "\n";);
}

void ItvAnalysis::post_analyze_stmt(const clang::ReturnStmt* return_stmt,
                                    AnalysisContext& ctx) const {
    LLVM_DEBUG(llvm::outs() << "ItvAnalysis::post analyze ReturnStmt: \n";
               return_stmt->dumpColor();
               llvm::outs() << "\n";);
    const auto* ret_val = return_stmt->getRetValue();
    if (ret_val == nullptr ||
        !ret_val->getType()->isIntegralOrEnumerationType()) {
        return;
    }
    auto state = ctx.get_state();
    auto ret_val_sexpr = state->get_stmt_sexpr(ret_val);
    if (!ret_val_sexpr) {
        return;
    }
    LLVM_DEBUG(llvm::outs() << "returning sexpr: ";
               ret_val_sexpr.value()->dump(llvm::outs());
               llvm::outs() << "\n";);

    if (auto znum = ret_val_sexpr.value()->get_as_znum()) {
        LLVM_DEBUG(llvm::outs() << "return znum: " << *znum << "\n";);
        return;
    }
    auto zvar = ret_val_sexpr.value()->get_as_zvariable();
    if (!zvar) {
        return;
    }

    LLVM_DEBUG(llvm::outs() << "return zvar: " << *zvar << "\n";
               if (auto zitv_dom_opt = state->get_ref< ZIntervalDom >()) {
                   const ZIntervalDom* zitv_dom = zitv_dom_opt.value();
                   llvm::outs() << "zinterval dom: ";
                   zitv_dom->dump(llvm::outs());
                   llvm::outs() << "\n";
                   llvm::outs() << "return zitv: ";
                   zitv_dom->get_value(*zvar).dump(llvm::outs());
                   llvm::outs() << "\n";
               });
}

void ItvAnalysis::EventHandler::handle(const ZVarAssignZVar& assign) const {
    LLVM_DEBUG(llvm::outs() << "ZVarAssignZVar: "; assign.dump(llvm::outs());
               llvm::outs() << "\n";);

    auto zitv = state->get_clone< ZIntervalDom >();
    zitv->assign_var(assign.x, assign.y);
    ctx->set_state(state->set< ZIntervalDom >(zitv));
}
void ItvAnalysis::EventHandler::handle(const ZVarAssignZNum& assign) const {
    LLVM_DEBUG(llvm::outs() << "ZVarAssignZNum: "; assign.dump(llvm::outs());
               llvm::outs() << "\n";);

    auto zitv = state->get_clone< ZIntervalDom >();
    zitv->assign_num(assign.x, assign.y);
    ctx->set_state(state->set< ZIntervalDom >(zitv));
}
void ItvAnalysis::EventHandler::handle(
    const ZVarAssignZLinearExpr& assign) const {
    LLVM_DEBUG(llvm::outs() << "ZVarAssignZLinearExpr: ";
               assign.dump(llvm::outs());
               llvm::outs() << "\n";);

    auto zitv = state->get_clone< ZIntervalDom >();
    zitv->assign_linear_expr(assign.x, assign.y);
    ctx->set_state(state->set< ZIntervalDom >(zitv));
}

void ItvAnalysis::EventHandler::handle(const ZVarAssignZCast& assign) const {}

void ItvAnalysis::EventHandler::handle(const QVarAssignQVar& assign) const {}

void ItvAnalysis::EventHandler::handle(const QVarAssignQNum& assign) const {}

void ItvAnalysis::EventHandler::handle(
    const QVarAssignQLinearExpr& assign) const {}
} // namespace knight::dfa