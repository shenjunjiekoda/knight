//===- binary_op_resolver.hpp ------------------------------------------===//
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

#include "analyzer/core/analysis/core/binary_op_resolver.hpp"
#include "analyzer/core/analysis/core/assign_resolver.hpp"
#include "analyzer/core/analysis/core/numerical_event.hpp"
#include "analyzer/core/symbol_manager.hpp"
#include "clang/AST/Expr.h"

#define DEBUG_TYPE "binary_op_resolver"

namespace knight::analyzer {

void BinaryOpResolver::resolve(
    const clang::BinaryOperator* binary_operator) const {
    handle_binary_operation(
        internal::BinaryOperationContext{binary_operator->getOpcode(),
                                         binary_operator->getLHS(),
                                         std::nullopt,
                                         binary_operator->getRHS(),
                                         std::nullopt,
                                         binary_operator->getType(),
                                         binary_operator});
}

void BinaryOpResolver::handle_binary_operation(
    internal::BinaryOperationContext bo_ctx) const {
    using enum clang::BinaryOperator::Opcode;

    bool is_int = bo_ctx.result_type->isIntegralOrEnumerationType();
    bool is_ref = bo_ctx.result_type->isReferenceType();
    bool is_ptr = bo_ctx.result_type->isPointerType();
    if (is_ref) {
        handle_ref_binary_operation(bo_ctx);
    }
    if (is_ptr) {
        handle_ptr_binary_operation(bo_ctx);
    }
    if (is_int) {
        handle_int_binary_operation(bo_ctx);
        return;
    }
    knight_log(llvm::outs() << "not int type binary operation!\n");
}

void BinaryOpResolver::handle_assign_binary_operation(
    internal::BinaryOperationContext bo_ctx) const {
    auto state = m_ctx->get_state();
    auto& sym_mgr = m_ctx->get_symbol_manager();
    internal::ExprRef lhs_expr = *bo_ctx.lhs_expr;
    auto rhs_expr_opt = bo_ctx.rhs_expr;
    auto op = bo_ctx.op;

    knight_assert_msg(clang::BinaryOperator::isAssignmentOp(op),
                      "shall be assign op here.");

    bool is_direct_assign = !clang::BinaryOperator::isCompoundAssignmentOp(op);
    SExprRef lhs_sexpr =
        is_direct_assign
            ? sym_mgr.get_symbol_conjured(lhs_expr,
                                          m_ctx->get_current_stack_frame())
            : state->get_stmt_sexpr(lhs_expr, m_ctx->get_current_stack_frame())
                  .value_or(nullptr);
    if (lhs_sexpr == nullptr) {
        knight_log(llvm::outs() << "lhs_sexpr is null!\n");
        return;
    }

    SExprRef rhs_sexpr =
        rhs_expr_opt ? state->get_stmt_sexpr_or_conjured(
                           *rhs_expr_opt, m_ctx->get_current_location_context())
                     : *bo_ctx.rhs_sexpr;

    knight_log_nl(llvm::outs() << "lhs_sexpr: "; lhs_sexpr->dump(llvm::outs());
                  llvm::outs() << "\nrhs_sexpr: ";
                  rhs_sexpr->dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto treg = state->get_region(lhs_expr, m_ctx->get_current_stack_frame());
    std::optional< const clang::Stmt* > stmt =
        treg ? std::nullopt : std::make_optional(bo_ctx.result_stmt);
    AssignResolver resolver(m_ctx, m_sym_resolver);
    m_ctx->set_state(resolver.resolve(internal::AssignmentContext{treg,
                                                                  stmt,
                                                                  rhs_expr_opt,
                                                                  lhs_sexpr,
                                                                  rhs_sexpr,
                                                                  op}));
}

void BinaryOpResolver::handle_int_non_assign_binary_operation(
    internal::BinaryOperationContext bo_ctx) const {
    auto state = m_ctx->get_state();
    auto& sym_mgr = m_ctx->get_symbol_manager();

    auto lhs_expr = bo_ctx.lhs_expr;
    auto rhs_expr = bo_ctx.rhs_expr;

    auto op = bo_ctx.op;
    knight_assert_msg(!clang::BinaryOperator::isAssignmentOp(op),
                      "cannot be assign op here.");

    SExprRef lhs_sexpr =
        lhs_expr ? state->get_stmt_sexpr_or_conjured(
                       *lhs_expr, m_ctx->get_current_location_context())
                 : *bo_ctx.lhs_sexpr;
    SExprRef rhs_sexpr =
        rhs_expr ? state->get_stmt_sexpr_or_conjured(
                       *rhs_expr, m_ctx->get_current_location_context())
                 : *bo_ctx.rhs_sexpr;

    knight_log_nl(llvm::outs() << "lhs_sexpr: "; lhs_sexpr->dump(llvm::outs());
                  llvm::outs() << "\nrhs_sexpr: ";
                  rhs_sexpr->dump(llvm::outs());
                  llvm::outs() << "\n";);

    SExprRef binary_sexpr = sym_mgr.get_binary_sym_expr(lhs_sexpr,
                                                        rhs_sexpr,
                                                        bo_ctx.op,
                                                        bo_ctx.result_type);
    knight_log(llvm::outs() << "binary_sexpr: " << *binary_sexpr << "\n");

    const auto* binary_conjured_sym =
        sym_mgr.get_symbol_conjured(bo_ctx.result_stmt,
                                    bo_ctx.result_type,
                                    m_ctx->get_current_stack_frame());

    ZVariable x(binary_conjured_sym);

    knight_log_nl(llvm::outs() << "zvar x: "; x.dump(llvm::outs());
                  llvm::outs() << "\n";);

    ZLinearExpr cstr(x);
    auto lhs_var = lhs_sexpr->get_as_zvariable();
    auto lhs_num = lhs_sexpr->get_as_znum();
    auto rhs_var = rhs_sexpr->get_as_zvariable();
    auto rhs_num = rhs_sexpr->get_as_znum();
    if (lhs_var && rhs_var) {
        LinearNumericalAssignEvent
            event(ZVarAssignBinaryVarVar{op, x, *lhs_var, *rhs_var}, state);
        m_sym_resolver->dispatch_event(event);

        cstr -= (*lhs_var + *rhs_var);
    } else if ((lhs_var && rhs_num) || (lhs_num && rhs_var)) {
        auto y = lhs_var ? *lhs_var : *rhs_var;
        auto z = lhs_var ? *rhs_num : *lhs_num;
        LinearNumericalAssignEvent event(ZVarAssignBinaryVarNum{op, x, y, z},
                                         state);
        m_sym_resolver->dispatch_event(event);

        cstr -= (y + z);
    } else if (auto zexpr = binary_sexpr->get_as_zexpr()) {
        if (auto znum = binary_sexpr->get_as_znum()) {
            LinearNumericalAssignEvent event(ZVarAssignZNum{x, *znum}, state);
            m_sym_resolver->dispatch_event(event);

            binary_sexpr = sym_mgr.get_scalar_int(*znum, bo_ctx.result_type);
            cstr -= *znum;
        } else if (auto zvar = binary_sexpr->get_as_zvariable()) {
            LinearNumericalAssignEvent event(ZVarAssignZVar{x, *zvar}, state);
            m_sym_resolver->dispatch_event(event);

            cstr -= *zvar;
        } else {
            LinearNumericalAssignEvent event(ZVarAssignZLinearExpr{x, *zexpr},
                                             state);
            m_sym_resolver->dispatch_event(event);

            cstr -= *zexpr;
        }
        state = state->add_zlinear_constraint(
            ZLinearConstraint(cstr, LinearConstraintKind::LCK_Equality));
    }

    knight_log(llvm::outs()
               << "binary sexpr: " << *binary_sexpr << " complexity: "
               << binary_sexpr->get_worst_complexity() << "\n");

    if (binary_sexpr->get_worst_complexity() > 1U) {
        state = state->set_stmt_sexpr(bo_ctx.result_stmt,
                                      m_ctx->get_current_stack_frame(),
                                      binary_conjured_sym);

        knight_log_nl(llvm::outs() << "set binary conjured: ";
                      binary_conjured_sym->dump(llvm::outs());
                      llvm::outs() << "\n");
    } else {
        state = state->set_stmt_sexpr(bo_ctx.result_stmt,
                                      m_ctx->get_current_stack_frame(),
                                      binary_sexpr);

        knight_log_nl(llvm::outs() << "set binary binary_sexpr: ";
                      binary_sexpr->dump(llvm::outs());
                      llvm::outs() << "\n");
    }
    knight_log(llvm::outs()
               << "after transfer binary here state: " << *state << "\n");

    m_ctx->set_state(state);
}

void BinaryOpResolver::handle_int_binary_operation(
    internal::BinaryOperationContext bo_ctx) const {
    auto op = bo_ctx.op;
    if (clang::BinaryOperator::isAssignmentOp(op)) {
        handle_assign_binary_operation(bo_ctx);
    } else {
        handle_int_non_assign_binary_operation(bo_ctx);
    }
}

void BinaryOpResolver::handle_ref_binary_operation(
    internal::BinaryOperationContext bo_ctx) const {
    // TODO(ref-binary-op): handle ref binary operation.
}

void BinaryOpResolver::handle_ptr_binary_operation(
    internal::BinaryOperationContext bo_ctx) const {
    // TODO(ptr-binary-op): handle ptr binary operation.
    auto op = bo_ctx.op;
    if (clang::BinaryOperator::isAssignmentOp(op)) {
        handle_assign_binary_operation(bo_ctx);
    }
}

} // namespace knight::analyzer