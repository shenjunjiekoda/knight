//===- itv_analysis.cpp ----------------------------------------------===//
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

#include "dfa/analysis/demo/itv_analysis.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/domain/interval_dom.hpp"
#include "util/log.hpp"

#define DEBUG_TYPE "ItvAnalysis"

namespace knight::dfa {

void ItvAnalysis::analyze_begin_function(
    [[maybe_unused]] AnalysisContext& ctx) const {
    knight_log_nl(llvm::outs()
                      << "ItvAnalysis::analyze_begin_function()\nstate:";
                  ctx.get_state()->dump(llvm::outs());
                  llvm::outs() << "\n";);
}

void ItvAnalysis::post_analyze_stmt(const clang::ReturnStmt* return_stmt,
                                    AnalysisContext& ctx) const {
    knight_log_nl(llvm::outs() << "ItvAnalysis::post analyze ReturnStmt: \n";
                  return_stmt->dumpColor();
                  llvm::outs() << "\n";);

    auto state = ctx.get_state();

    const auto* ret_val = return_stmt->getRetValue();
    if (ret_val == nullptr ||
        !ret_val->getType()->isIntegralOrEnumerationType()) {
        return;
    }
    auto ret_val_sexpr = state->get_stmt_sexpr(ret_val);
    if (!ret_val_sexpr) {
        return;
    }
    knight_log_nl(llvm::outs() << "returning sexpr: ";
                  ret_val_sexpr.value()->dump(llvm::outs());
                  llvm::outs() << "\n";);

    if (auto znum = ret_val_sexpr.value()->get_as_znum()) {
        knight_log_nl(llvm::outs() << "return znum: " << *znum << "\n";);
        return;
    }
    auto zvar = ret_val_sexpr.value()->get_as_zvariable();
    if (!zvar) {
        return;
    }

    knight_log_nl(llvm::outs() << "return zvar: " << *zvar << "\n";
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
    knight_log_nl(llvm::outs() << "ZVarAssignZVar: "; assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zitv = state->get_clone< ZIntervalDom >();
    zitv->assign_var(assign.x, assign.y);
    ctx->set_state(state->set< ZIntervalDom >(zitv));
}
void ItvAnalysis::EventHandler::handle(const ZVarAssignZNum& assign) const {
    knight_log_nl(llvm::outs() << "ZVarAssignZNum: "; assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zitv = state->get_clone< ZIntervalDom >();
    zitv->assign_num(assign.x, assign.y);
    ctx->set_state(state->set< ZIntervalDom >(zitv));
}
void ItvAnalysis::EventHandler::handle(
    const ZVarAssignZLinearExpr& assign) const {
    knight_log_nl(llvm::outs() << "ZVarAssignZLinearExpr: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zitv = state->get_clone< ZIntervalDom >();
    zitv->assign_linear_expr(assign.x, assign.y);
    ctx->set_state(state->set< ZIntervalDom >(zitv));
}

void ItvAnalysis::EventHandler::handle(const ZVarAssignZCast& assign) const {
    knight_log_nl(llvm::outs() << "ZVarAssignZCast: ";
                  assign.dump(llvm::outs()););

    auto zitv = state->get_clone< ZIntervalDom >();
    zitv->assign_cast(assign.dst_type,
                      assign.dst_bit_width,
                      assign.x,
                      assign.y);
    ctx->set_state(state->set< ZIntervalDom >(zitv));
}

void ItvAnalysis::EventHandler::handle(
    const ZVarAssignBinaryVarVar& assign) const {
    knight_log_nl(llvm::outs() << "ZVarAssignBinaryVarVar: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zitv = state->get_clone< ZIntervalDom >();
    zitv->assign_binary_var_var_impl(assign.op, assign.x, assign.y, assign.z);
    ctx->set_state(state->set< ZIntervalDom >(zitv));
}
void ItvAnalysis::EventHandler::handle(
    const ZVarAssignBinaryVarNum& assign) const {
    knight_log_nl(llvm::outs() << "ZVarAssignBinaryVarNum: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zitv = state->get_clone< ZIntervalDom >();
    zitv->assign_binary_var_num_impl(assign.op, assign.x, assign.y, assign.z);

    ctx->set_state(state->set< ZIntervalDom >(zitv));
}

void ItvAnalysis::EventHandler::handle(const QVarAssignQVar& assign) const {}

void ItvAnalysis::EventHandler::handle(const QVarAssignQNum& assign) const {}

void ItvAnalysis::EventHandler::handle(
    const QVarAssignQLinearExpr& assign) const {}
} // namespace knight::dfa