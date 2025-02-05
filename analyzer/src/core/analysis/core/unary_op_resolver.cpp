//===- unary_op_resolver.cpp ------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements the uanry op resolver.
//
//===------------------------------------------------------------------===//

#include "analyzer/core/analysis/core/unary_op_resolver.hpp"
#include "analyzer/core/analysis/core/binary_op_resolver.hpp"
#include "common/util/log.hpp"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "unary_op_resolver"

namespace knight::analyzer {

void UnaryOpResolver::resolve(
    const clang::UnaryOperator* unary_operator) const {
    auto type = unary_operator->getType();
    if (type->isIntegralOrEnumerationType()) {
        handle_int_unary_operation(unary_operator);
        return;
    }
    if (type->isPointerType()) {
        handle_ptr_unary_operation(unary_operator);
        return;
    }
    knight_log(llvm::outs() << "unhandled unary operator type: " << type);
}

void UnaryOpResolver::handle_ptr_unary_operation(
    const clang::UnaryOperator* unary_operator) const {
    knight_log_nl(llvm::outs() << "handle ptr unary operator: ";
                  unary_operator->dumpColor());
    auto type = unary_operator->getType();
    auto state = m_ctx->get_state();
    auto* operand_expr = unary_operator->getSubExpr();
    SExprRef operand_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(operand_expr,
                                         m_ctx->get_current_location_context());
    auto& sym_mgr = m_ctx->get_symbol_manager();
    auto unary_op = unary_operator->getOpcode();
    switch (unary_op) {
        using enum clang::UnaryOperatorKind;
        case clang::UO_AddrOf: {
            // TODO (stmt-pt, region-pt)
            knight_log_nl(llvm::outs() << "addr op\n";);

            auto treg = state->get_region(operand_expr,
                                          m_ctx->get_current_stack_frame());
            if (!treg) {
                auto treg = operand_sexpr->get_as_region();
            }
            if (treg) {
                const auto* reg_addr_val = sym_mgr.get_region_addr(*treg);
                state = state->set_stmt_sexpr(unary_operator,
                                              m_ctx->get_current_stack_frame(),
                                              reg_addr_val);
                knight_log_nl(
                    llvm::outs() << "set addr of expr: `";
                    operand_expr->printPretty(llvm::outs(),
                                              nullptr,
                                              m_ctx->get_ast_context()
                                                  .getPrintingPolicy());
                    llvm::outs() << "` , region: `";
                    treg.value()->dump(llvm::outs());
                    llvm::outs() << "`, to scalar val: ";
                    reg_addr_val->dump(llvm::outs());
                    llvm::outs() << "\n";);
            }
        } break;
        default:
            break;
    }
    m_ctx->set_state(state);
}

void UnaryOpResolver::handle_int_unary_operation(
    const clang::UnaryOperator* unary_operator) const {
    auto type = unary_operator->getType();
    auto state = m_ctx->get_state();
    auto* operand_expr = unary_operator->getSubExpr();
    auto& sym_mgr = m_ctx->get_symbol_manager();
    auto unary_op = unary_operator->getOpcode();
    switch (unary_op) {
        using enum clang::UnaryOperatorKind;
        case clang::UO_LNot: {
            BinaryOpResolver resolver(m_ctx, m_sym_resolver);
            resolver.handle_binary_operation(
                internal::BinaryOperationContext{clang::BO_EQ,
                                                 std::nullopt,
                                                 sym_mgr.get_scalar_int(ZNum(0),
                                                                        type),
                                                 operand_expr,
                                                 std::nullopt,
                                                 unary_operator->getType(),
                                                 unary_operator});
            return;
        }
        case clang::UO_Plus:
            [[fallthrough]];
        case clang::UO_Minus: {
            BinaryOpResolver resolver(m_ctx, m_sym_resolver);
            resolver.handle_binary_operation(
                internal::BinaryOperationContext{unary_op == UO_Plus
                                                     ? clang::BO_Add
                                                     : clang::BO_Sub,
                                                 std::nullopt,
                                                 sym_mgr.get_scalar_int(ZNum(0),
                                                                        type),
                                                 operand_expr,
                                                 std::nullopt,
                                                 unary_operator->getType(),
                                                 unary_operator});
            return;
        } break;
        case clang::UO_PostInc:
            [[fallthrough]];
        case clang::UO_PreInc: {
            BinaryOpResolver resolver(m_ctx, m_sym_resolver);
            resolver.handle_binary_operation(
                internal::BinaryOperationContext{clang::BO_AddAssign,
                                                 operand_expr,
                                                 std::nullopt,
                                                 std::nullopt,
                                                 sym_mgr.get_scalar_int(ZNum(1),
                                                                        type),
                                                 unary_operator->getType(),
                                                 unary_operator});
            return;
        }
        case clang::UO_PostDec:
            [[fallthrough]];
        case clang::UO_PreDec: {
            BinaryOpResolver resolver(m_ctx, m_sym_resolver);
            resolver.handle_binary_operation(
                internal::BinaryOperationContext{clang::BO_SubAssign,
                                                 operand_expr,
                                                 std::nullopt,
                                                 std::nullopt,
                                                 sym_mgr.get_scalar_int(ZNum(1),
                                                                        type),
                                                 unary_operator->getType(),
                                                 unary_operator});
            return;
        }
        default:
            break;
    }
}

} // namespace knight::analyzer