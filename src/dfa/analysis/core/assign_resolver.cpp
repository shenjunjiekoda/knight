//===- assign_resolver.cpp --------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements the assign resolver.
//
//===------------------------------------------------------------------===//

#include "dfa/analysis/core/assign_resolver.hpp"
#include "util/log.hpp"

#define DEBUG_TYPE "assign_resolver"

namespace knight::dfa {

void AssignResolver::handle_ptr_assign(internal::AssignmentContext assign_ctx,
                                       SymbolRef res_sym,
                                       bool is_direct_assign,
                                       clang::BinaryOperator::Opcode op,
                                       ProgramStateRef& state,
                                       SExprRef& binary_sexpr) const {
    auto type = assign_ctx.rhs_sexpr->get_type();
    auto& sym_mgr = m_ctx->get_symbol_manager();
    if (!is_direct_assign) {
        knight_unreachable("indirect ptr assign not supported yet");
        (void)op;
    }
    auto region_def = llvm::dyn_cast< RegionDef >(binary_sexpr);
    auto region_addr_val = llvm::dyn_cast< RegionAddr >(binary_sexpr);
}

void AssignResolver::handle_int_assign(internal::AssignmentContext assign_ctx,
                                       SymbolRef res_sym,
                                       bool is_direct_assign,
                                       clang::BinaryOperator::Opcode op,
                                       ProgramStateRef& state,
                                       SExprRef& binary_sexpr) const {
    auto type = assign_ctx.rhs_sexpr->get_type();
    auto& sym_mgr = m_ctx->get_symbol_manager();
    ZVariable x(res_sym);

    ZLinearExpr cstr;
    cstr += x;

    knight_log_nl(llvm::outs() << "zvar x: "; x.dump(llvm::outs());
                  llvm::outs() << "\n";);
    auto lhs_var = assign_ctx.lhs_sexpr != nullptr
                       ? assign_ctx.lhs_sexpr->get_as_zvariable()
                       : std::nullopt;
    auto lhs_num = assign_ctx.lhs_sexpr != nullptr
                       ? assign_ctx.lhs_sexpr->get_as_znum()
                       : std::nullopt;
    auto rhs_var = assign_ctx.rhs_sexpr->get_as_zvariable();
    auto rhs_num = assign_ctx.rhs_sexpr->get_as_znum();
    if (!is_direct_assign && lhs_var && rhs_var) {
        LinearNumericalAssignEvent
            event(ZVarAssignBinaryVarVar{op, x, *lhs_var, *rhs_var}, state);
        m_sym_resolver->dispatch_event(event);

        cstr -= (*lhs_var + *rhs_var);
    } else if (!is_direct_assign &&
               ((lhs_var && rhs_num) || (lhs_num && rhs_var))) {
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

            binary_sexpr = sym_mgr.get_scalar_int(*znum, type);
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
    } else if (auto rhs_expr = assign_ctx.rhs_expr) {
        ZVariable y(
            sym_mgr.get_symbol_conjured(*rhs_expr,
                                        m_ctx->get_current_stack_frame()));
        LinearNumericalAssignEvent event(ZVarAssignZVar{x, y}, state);
        m_sym_resolver->dispatch_event(event);
        cstr -= y;
    } else {
        knight_log(llvm::outs() << "unhandled case!\n");
        return;
    }
    state = state->add_zlinear_constraint(
        ZLinearConstraint(cstr, LinearConstraintKind::LCK_Equality));
}

ProgramStateRef AssignResolver::resolve(
    internal::AssignmentContext assign_ctx) const {
    auto op = assign_ctx.op;
    bool is_direct_assign = !clang::BinaryOperator::isCompoundAssignmentOp(op);

    knight_assert_msg(assign_ctx.rhs_sexpr != nullptr,
                      "rhs sexpr shall be nonnull in assignment context");
    knight_assert_msg(assign_ctx.lhs_sexpr != nullptr || is_direct_assign,
                      "lhs sexpr shall be nonnull in indirect assignment "
                      "context");
    knight_assert_msg((!assign_ctx.stmt && assign_ctx.treg) ||
                          (assign_ctx.stmt && !assign_ctx.treg),
                      "either stmt or treg should be set in assignment "
                      "context");

    auto state = m_ctx->get_state();
    auto& sym_mgr = m_ctx->get_symbol_manager();
    auto type = assign_ctx.rhs_sexpr->get_type();
    SExprRef binary_sexpr = nullptr;
    if (is_direct_assign) {
        binary_sexpr = assign_ctx.rhs_sexpr;
    } else {
        op = clang::BinaryOperator::getOpForCompoundAssignment(op);
        binary_sexpr = sym_mgr.get_binary_sym_expr(assign_ctx.lhs_sexpr,
                                                   assign_ctx.rhs_sexpr,
                                                   op,
                                                   type);
    }

    SymbolRef res_sym = nullptr;
    if (assign_ctx.treg) {
        res_sym = sym_mgr.get_region_def(*assign_ctx.treg,
                                         m_ctx->get_current_location_context());
    } else {
        res_sym = sym_mgr.get_symbol_conjured(*assign_ctx.stmt,
                                              type,
                                              m_ctx->get_current_stack_frame());
    }

    knight_log_nl(
        llvm::outs() << "lhs_sexpr: "; if (assign_ctx.lhs_sexpr != nullptr) {
            assign_ctx.lhs_sexpr->dump(llvm::outs());
        } else {
            llvm::outs() << "nullptr";
        } llvm::outs() << "\nrhs_sexpr: "
                       << *assign_ctx.rhs_sexpr << "\n";
        llvm::outs() << "binary_sexpr: " << *binary_sexpr << "\n";);

    if (type->isPointerType()) {
        handle_ptr_assign(assign_ctx,
                          res_sym,
                          is_direct_assign,
                          op,
                          state,
                          binary_sexpr);
    } else if (type->isIntegralOrEnumerationType()) {
        handle_int_assign(assign_ctx,
                          res_sym,
                          is_direct_assign,
                          op,
                          state,
                          binary_sexpr);
    }

    if (assign_ctx.treg) {
        knight_log_nl(llvm::outs() << "\nset region: ";
                      assign_ctx.treg.value()->dump(llvm::outs());
                      llvm::outs() << " to new_def: ";
                      res_sym->dump(llvm::outs());
                      llvm::outs() << "\n");

        state = state->set_region_def(*assign_ctx.treg,
                                      m_ctx->get_current_stack_frame(),
                                      cast< RegionDef >(res_sym));
    } else {
        state = state->set_stmt_sexpr(*assign_ctx.stmt,
                                      m_ctx->get_current_stack_frame(),
                                      res_sym);
    }

    return state;
}

} // namespace knight::dfa