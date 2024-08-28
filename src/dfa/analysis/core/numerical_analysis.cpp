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

void NumericalAnalysis::EventHandler::handle(
    const ZVarAssignZVar& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZVar: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zitv = input_state->get_zdom_clone();
    zitv->assign_var(assign.x, assign.y);
    output_state = input_state->set_zdom(zitv);
}
void NumericalAnalysis::EventHandler::handle(
    const ZVarAssignZNum& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZNum: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zitv = input_state->get_zdom_clone();
    zitv->assign_num(assign.x, assign.y);
    output_state = input_state->set_zdom(zitv);
}
void NumericalAnalysis::EventHandler::handle(
    const ZVarAssignZLinearExpr& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZLinearExpr: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zitv = input_state->get_zdom_clone();
    zitv->assign_linear_expr(assign.x, assign.y);
    output_state = input_state->set_zdom(zitv);
}

void NumericalAnalysis::EventHandler::handle(
    const ZVarAssignZCast& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignZCast: ";
                  assign.dump(llvm::outs()););

    auto zitv = input_state->get_zdom_clone();
    zitv->assign_cast(assign.dst_type,
                      assign.dst_bit_width,
                      assign.x,
                      assign.y);
    output_state = input_state->set_zdom(zitv);
}

void NumericalAnalysis::EventHandler::handle(
    const ZVarAssignBinaryVarVar& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignBinaryVarVar: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zitv = input_state->get_zdom_clone();
    zitv->assign_binary_var_var(assign.op, assign.x, assign.y, assign.z);
    output_state = input_state->set_zdom(zitv);
}
void NumericalAnalysis::EventHandler::handle(
    const ZVarAssignBinaryVarNum& assign) const {
    knight_log_nl(llvm::outs() << "Event ZVarAssignBinaryVarNum: ";
                  assign.dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto zitv = input_state->get_zdom_clone();
    zitv->assign_binary_var_num(assign.op, assign.x, assign.y, assign.z);
    output_state = input_state->set_zdom(zitv);
}

void NumericalAnalysis::EventHandler::handle(
    const QVarAssignQVar& assign) const {}

void NumericalAnalysis::EventHandler::handle(
    const QVarAssignQNum& assign) const {}

void NumericalAnalysis::EventHandler::handle(
    const QVarAssignQLinearExpr& assign) const {}
} // namespace knight::dfa