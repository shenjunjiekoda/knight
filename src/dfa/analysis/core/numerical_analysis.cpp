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

void NumericalAnalysis::LinearNumericalAssignEventHandler::handle(
    const ZVarAssignZVar& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZVar: " << assign << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assign_var(assign.x, assign.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearNumericalAssignEventHandler::handle(
    const ZVarAssignZNum& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZNum: " << assign << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assign_num(assign.x, assign.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearNumericalAssignEventHandler::handle(
    const ZVarAssignZLinearExpr& assign) const {
    knight_log_nl(llvm::outs()
                      << "Event ZVarAssignZLinearExpr: " << assign << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assign_linear_expr(assign.x, assign.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearNumericalAssignEventHandler::handle(
    const ZVarAssignZCast& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZCast: " << assign << "\n");

    auto zdom = state->get_zdom_clone();
    zdom->assign_cast(assign.dst_type,
                      assign.dst_bit_width,
                      assign.x,
                      assign.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearNumericalAssignEventHandler::handle(
    const ZVarAssignBinaryVarVar& assign) const {
    knight_log_nl(llvm::outs()
                      << "Event ZVarAssignBinaryVarVar: " << assign << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assign_binary_var_var(assign.op, assign.x, assign.y, assign.z);
    state = state->set_zdom(zdom);
}
void NumericalAnalysis::LinearNumericalAssignEventHandler::handle(
    const ZVarAssignBinaryVarNum& assign) const {
    knight_log_nl(llvm::outs()
                      << "Event ZVarAssignBinaryVarNum: " << assign << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assign_binary_var_num(assign.op, assign.x, assign.y, assign.z);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearNumericalAssumptionEventHandler::handle(
    const PredicateZVarZNum& pred) const {
    knight_log_nl(llvm::outs() << "Event PredicateZVarZNum: " << pred << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assume_predicate_var_num(pred.op, pred.x, pred.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearNumericalAssumptionEventHandler::handle(
    const PredicateZVarZVar& pred) const {
    knight_log_nl(llvm::outs() << "Event PredicateZVarZVar: " << pred << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->assume_predicate_var_var(pred.op, pred.x, pred.y);
    state = state->set_zdom(zdom);
}

void NumericalAnalysis::LinearNumericalAssumptionEventHandler::handle(
    const GeneralLinearConstraint& cstr) const {
    knight_log_nl(llvm::outs()
                      << "Event GeneralLinearConstraint: " << cstr << "\n";);

    auto zdom = state->get_zdom_clone();
    zdom->apply_linear_constraint(cstr.cstr);
    state = state->set_zdom(zdom);
}

} // namespace knight::dfa
