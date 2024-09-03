//===- numerical_analysis.cpp -----------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements an numerical analysis
//
//===------------------------------------------------------------------===//

#include "dfa/analysis/core/numerical_analysis.hpp"
#include "dfa/constraint/linear.hpp"
#include "util/log.hpp"

#define DEBUG_TYPE "NumericalAnalysis"

namespace knight::dfa {

void NumericalAnalysis::LinearAssignEventHandler::handle(
    const ZVarAssignZVar& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZVar: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assign_var(assign.x, assign.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearAssignEventHandler::handle(
    const ZVarAssignZNum& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZNum: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assign_num(assign.x, assign.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearAssignEventHandler::handle(
    const ZVarAssignZLinearExpr& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZLinearExpr: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assign_linear_expr(assign.x, assign.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearAssignEventHandler::handle(
    const ZVarAssignZCast& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZCast: ";
                  assign.dump(llvm::outs()););

    auto zdom = state->get_zdom_clone();
    zdom->assign_cast(assign.dst_type,
                      assign.dst_bit_width,
                      assign.x,
                      assign.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearAssignEventHandler::handle(
    const ZVarAssignBinaryVarVar& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignBinaryVarVar: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assign_binary_var_var(assign.op, assign.x, assign.y, assign.z);
    state = state->set_zdom(zdom);
}
void NumericalAnalysis::LinearAssignEventHandler::handle(
    const ZVarAssignBinaryVarNum& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignBinaryVarNum: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assign_binary_var_num(assign.op, assign.x, assign.y, assign.z);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearAssumptionEventHandler::handle(
    const PredicateZVarZNum& pred) const {
    knight_log_nl(llvm::outs() << "Event PredicateZVarZNum: ";
                  pred.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assume_predicate_var_num(pred.op, pred.x, pred.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearAssumptionEventHandler::handle(
    const PredicateZVarZVar& pred) const {
    knight_log_nl(llvm::outs() << "Event PredicateZVarZVar: ";
                  pred.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assume_predicate_var_var(pred.op, pred.x, pred.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearAssumptionEventHandler::handle(
    const GeneralLinearConstraint& cstr) const {
    knight_log_nl(llvm::outs() << "Event GeneralLinearConstraint: ";
                  cstr.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->apply_linear_constraint(cstr.cstr);
    state = state->set_zdom(zdom);
}

} // namespace knight::dfa
