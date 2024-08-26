//===- symbol_resolver.cpp --------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements the symbol resolver.
//
//===------------------------------------------------------------------===//

#include "dfa/analysis/core/symbol_resolver.hpp"
#include <optional>
#include "clang/AST/Expr.h"
#include "dfa/analysis/core/numerical_event.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/region/region.hpp"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "util/log.hpp"

#define DEBUG_TYPE "SymbolResolver"

namespace knight::dfa {

namespace {

bool is_loading_lvalue(const clang::Expr* expr) {
    const auto* implicit_cast_expr =
        llvm::dyn_cast_or_null< clang::ImplicitCastExpr >(expr);
    return implicit_cast_expr != nullptr &&
           implicit_cast_expr->getCastKind() == clang::CK_LValueToRValue;
}

} // anonymous namespace

void SymbolResolver::VisitIntegerLiteral(
    const clang::IntegerLiteral* integer_literal) const {
    auto state = m_ctx->get_state();
    auto ap_int = integer_literal->getValue();
    auto aps_int = llvm::APSInt(ap_int,
                                integer_literal->getType()
                                    ->isUnsignedIntegerOrEnumerationType());

    SymbolManager& symbol_manager = m_ctx->get_symbol_manager();
    const auto* scalar =
        symbol_manager.get_scalar_int(ZNum(aps_int.getZExtValue()),
                                      integer_literal->getType());

    knight_log_nl(llvm::outs() << "set integer literal: ";
                  integer_literal->printPretty(llvm::outs(),
                                               nullptr,
                                               m_ctx->get_ast_context()
                                                   .getPrintingPolicy());
                  llvm::outs() << " to scalar: ";
                  scalar->dump(llvm::outs());
                  llvm::outs() << "\n";);

    m_ctx->set_state(state->set_stmt_sexpr(integer_literal, scalar));
}

void SymbolResolver::VisitFloatingLiteral(
    const clang::FloatingLiteral* floating_literal) const {
    auto state = m_ctx->get_state();
    auto ap_float = floating_literal->getValue();
    auto aps_float = llvm::APFloat(ap_float);

    SymbolManager& symbol_manager = m_ctx->get_symbol_manager();
    const auto* scalar =
        symbol_manager.get_scalar_float(QNum(aps_float.convertToDouble()),
                                        floating_literal->getType());

    knight_log_nl(llvm::outs() << "SymbolResolver::VisitFloatLiteral: "
                               << "set floating literal: ";
                  floating_literal->printPretty(llvm::outs(),
                                                nullptr,
                                                m_ctx->get_ast_context()
                                                    .getPrintingPolicy());
                  llvm::outs() << " to scalar: ";
                  scalar->dump(llvm::outs());
                  llvm::outs() << "\n";);

    m_ctx->set_state(state->set_stmt_sexpr(floating_literal, scalar));
}

void SymbolResolver::VisitUnaryOperator(
    const clang::UnaryOperator* unary_operator) const {
    auto state = m_ctx->get_state();
    auto& sym_mgr = m_ctx->get_symbol_manager();
    auto* operand_expr = unary_operator->getSubExpr();
    auto type = unary_operator->getType();
    unsigned bit_width = m_ctx->get_ast_context().getTypeSize(type);

    SExprRef operand_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(operand_expr,
                                         m_ctx->get_current_location_context());
    if (auto y_var = operand_sexpr->get_as_zvariable()) {
        ZVariable x(
            sym_mgr.get_symbol_conjured(unary_operator,
                                        m_ctx->get_current_stack_frame()));
        dispatch_event(
            LinearAssignEvent(ZVarAssignZCast{bit_width, type, x, *y_var},
                              state,
                              m_ctx));
    }
    SExprRef unary_sexpr =
        sym_mgr.get_unary_sym_expr(operand_sexpr,
                                   unary_operator->getOpcode(),
                                   unary_operator->getType());

    knight_log_nl(llvm::outs() << "set unary operator: ";
                  unary_operator->printPretty(llvm::outs(),
                                              nullptr,
                                              m_ctx->get_ast_context()
                                                  .getPrintingPolicy());
                  llvm::outs() << " to sexpr: ";
                  unary_sexpr->dump(llvm::outs());
                  llvm::outs() << "\n";);

    state = state->set_stmt_sexpr(unary_operator, unary_sexpr);
    m_ctx->set_state(state);
}

std::pair< SExprRef, ZLinearExpr > SymbolResolver::handle_assign_sexpr_and_cstr(
    AssignmentContext assign_ctx) const {
    knight_assert_msg(assign_ctx.rhs_sexpr != nullptr,
                      "rhs sexpr shall be nonnull in assignment context");
    auto op = assign_ctx.op;
    bool is_direct_assign = !clang::BinaryOperator::isCompoundAssignmentOp(op);
    auto state = m_ctx->get_state();
    auto& sym_mgr = m_ctx->get_symbol_manager();

    SExprRef binary_sexpr = nullptr;
    if (is_direct_assign) {
        binary_sexpr = assign_ctx.rhs_sexpr;
    } else {
        op = clang::BinaryOperator::getOpForCompoundAssignment(op);
        binary_sexpr = sym_mgr.get_binary_sym_expr(assign_ctx.lhs_sexpr,
                                                   assign_ctx.rhs_sexpr,
                                                   op,
                                                   assign_ctx.type);
    }
    ZLinearExpr cstr;

    if (!assign_ctx.treg) {
        knight_log(llvm::outs() << "no typed region for lhs!\n");
        return {binary_sexpr, cstr};
    }

    const auto* new_reg_def_val =
        sym_mgr.get_region_sym_val(*assign_ctx.treg,
                                   m_ctx->get_current_location_context());

    if (assign_ctx.is_int) {
        ZVariable x(new_reg_def_val);
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
            dispatch_event(LinearAssignEvent(ZVarAssignBinaryVarVar{op,
                                                                    x,
                                                                    *lhs_var,
                                                                    *rhs_var},
                                             state,
                                             m_ctx));
            if (m_ctx->is_state_changed()) {
                state = m_ctx->get_state();
            }
            cstr -= (*lhs_var + *rhs_var);
        } else if (!is_direct_assign &&
                   ((lhs_var && rhs_num) || (lhs_num && rhs_var))) {
            auto y = lhs_var ? *lhs_var : *rhs_var;
            auto z = lhs_var ? *rhs_num : *lhs_num;
            dispatch_event(
                LinearAssignEvent(ZVarAssignBinaryVarNum{op, x, y, z},
                                  state,
                                  m_ctx));
            if (m_ctx->is_state_changed()) {
                state = m_ctx->get_state();
            }
            cstr -= (y + z);
        } else if (auto zexpr = binary_sexpr->get_as_zexpr()) {
            if (auto znum = binary_sexpr->get_as_znum()) {
                dispatch_event(
                    LinearAssignEvent(ZVarAssignZNum{x, *znum}, state, m_ctx));
                binary_sexpr = sym_mgr.get_scalar_int(*znum, assign_ctx.type);
                cstr -= *znum;
            } else if (auto zvar = binary_sexpr->get_as_zvariable()) {
                dispatch_event(
                    LinearAssignEvent(ZVarAssignZVar{x, *zvar}, state, m_ctx));
                cstr -= *zvar;
            } else {
                dispatch_event(
                    LinearAssignEvent(ZVarAssignZLinearExpr{x, *zexpr},
                                      state,
                                      m_ctx));
                cstr -= *zexpr;
            }
            if (m_ctx->is_state_changed()) {
                state = m_ctx->get_state();
            }
        }
    } else {
        llvm::errs() << "floating point not implemented for now!\n";
    }

    if (assign_ctx.treg) {
        knight_log_nl(llvm::outs() << "\nset new region: ";
                      assign_ctx.treg.value()->dump(llvm::outs());
                      llvm::outs() << " to new_def: ";
                      new_reg_def_val->dump(llvm::outs());
                      llvm::outs() << "\n");

        state = state->set_region_def(*assign_ctx.treg, new_reg_def_val);
    }
    m_ctx->set_state(state);

    return {new_reg_def_val, cstr};
}

void SymbolResolver::VisitBinaryOperator(
    const clang::BinaryOperator* binary_operator) const {
    using enum clang::BinaryOperator::Opcode;

    auto state = m_ctx->get_state();
    auto& sym_mgr = m_ctx->get_symbol_manager();

    auto* lhs_expr = binary_operator->getLHS();
    auto* rhs_expr = binary_operator->getRHS();

    bool is_int = binary_operator->getType()->isIntegralOrEnumerationType();
    bool is_float = binary_operator->getType()->isFloatingType();
    if (!is_int && !is_float) {
        knight_log(llvm::outs() << "not int or float!\n");
        return;
    }
    bool is_assign =
        clang::BinaryOperator::isAssignmentOp(binary_operator->getOpcode());

    SExprRef lhs_sexpr =
        is_assign
            ? sym_mgr.get_symbol_conjured(lhs_expr,
                                          m_ctx->get_current_stack_frame())
            : state->get_stmt_sexpr_or_conjured(
                  lhs_expr, m_ctx->get_current_location_context());

    SExprRef rhs_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(rhs_expr,
                                         m_ctx->get_current_location_context());
    knight_log_nl(llvm::outs() << "lhs_sexpr: "; lhs_sexpr->dump(llvm::outs());
                  llvm::outs() << "\nrhs_sexpr: ";
                  rhs_sexpr->dump(llvm::outs());
                  llvm::outs() << "\n";);

    SExprRef binary_sexpr = nullptr;
    if (is_assign) {
        auto treg =
            state->get_region(lhs_expr, m_ctx->get_current_stack_frame());
        auto [binary_sexpr_, cstr] = handle_assign_sexpr_and_cstr(
            AssignmentContext{is_int,
                              treg,
                              binary_operator->getType(),
                              lhs_sexpr,
                              rhs_sexpr,
                              binary_operator->getOpcode()});
        if (m_ctx->is_state_changed()) {
            state = m_ctx->get_state();
        }
        binary_sexpr = binary_sexpr_;
        state = state->add_zlinear_constraint(
            ZLinearConstraint(cstr, LinearConstraintKind::LCK_Equality));
    } else {
        binary_sexpr = sym_mgr.get_binary_sym_expr(lhs_sexpr,
                                                   rhs_sexpr,
                                                   binary_operator->getOpcode(),
                                                   binary_operator->getType());
        if (is_int) {
            ZVariable x(
                sym_mgr.get_symbol_conjured(binary_operator,
                                            m_ctx->get_current_stack_frame()));
            knight_log_nl(llvm::outs() << "zvar x: "; x.dump(llvm::outs());
                          llvm::outs() << "\n";);

            if (auto zexpr = binary_sexpr->get_as_zexpr()) {
                ZLinearExpr cstr(x);
                if (auto znum = binary_sexpr->get_as_znum()) {
                    dispatch_event(LinearAssignEvent(ZVarAssignZNum{x, *znum},
                                                     state,
                                                     m_ctx));
                    binary_sexpr =
                        sym_mgr.get_scalar_int(*znum,
                                               binary_operator->getType());
                    cstr -= *znum;
                } else if (auto zvar = binary_sexpr->get_as_zvariable()) {
                    dispatch_event(LinearAssignEvent(ZVarAssignZVar{x, *zvar},
                                                     state,
                                                     m_ctx));
                    cstr -= *zvar;
                } else {
                    dispatch_event(
                        LinearAssignEvent(ZVarAssignZLinearExpr{x, *zexpr},
                                          state,
                                          m_ctx));
                    cstr -= *zexpr;
                }
                if (m_ctx->is_state_changed()) {
                    state = m_ctx->get_state();
                }
                state = state->add_zlinear_constraint(
                    ZLinearConstraint(cstr,
                                      LinearConstraintKind::LCK_Equality));
            }
        }
        // TODO(floating-bo): support fp
    }

    state = state->set_stmt_sexpr(binary_operator, binary_sexpr);

    knight_log_nl(llvm::outs() << "set binary_sexpr: ";
                  binary_sexpr->dump(llvm::outs());
                  llvm::outs() << "\n";);

    m_ctx->set_state(state);
}

void SymbolResolver::VisitDeclStmt(const clang::DeclStmt* decl_stmt) const {
    auto state = m_ctx->get_state();
    SExprRef stmt_sexpr = nullptr;
    for (const auto* decl : decl_stmt->decls()) {
        stmt_sexpr = nullptr;

        // TODO(decl-region): handle other decl types.
        if (const auto* var_decl = llvm::dyn_cast< clang::VarDecl >(decl)) {
            if (!var_decl->hasInit()) {
                // We don't need to handle uninitialized variables here.
                continue;
            }
            const auto* init_expr = var_decl->getInit();
            auto init_sexpr_opt = state->get_stmt_sexpr(init_expr);
            if (!init_sexpr_opt.has_value()) {
                continue;
            }

            if (auto treg =
                    state->get_region(var_decl,
                                      m_ctx->get_current_stack_frame())) {
                auto type = treg.value()->get_value_type();
                auto [sexpr, cstr] = handle_assign_sexpr_and_cstr(
                    AssignmentContext{type->isIntegralOrEnumerationType(),
                                      treg,
                                      type,
                                      nullptr,
                                      init_sexpr_opt.value()});
                if (m_ctx->is_state_changed()) {
                    state = m_ctx->get_state();
                }
                state = state->add_zlinear_constraint(
                    ZLinearConstraint(cstr,
                                      LinearConstraintKind::LCK_Equality));

                init_sexpr_opt = sexpr;

            } else {
                llvm::errs() << "no typed region for decl!\n";
            }
            stmt_sexpr = *init_sexpr_opt;
        }
    }

    if (stmt_sexpr != nullptr) {
        state = state->set_stmt_sexpr(decl_stmt, stmt_sexpr);

        knight_log_nl(llvm::outs() << "set decl_stmt sexpr to: ";
                      stmt_sexpr->dump(llvm::outs());
                      llvm::outs() << "\n";);
    }

    m_ctx->set_state(state);
}

void SymbolResolver::handle_load(const clang::Expr* load_expr) const {
    const auto* imp_cast_expr = dyn_cast< clang::ImplicitCastExpr >(load_expr);
    const auto* src = imp_cast_expr->getSubExpr();
    const auto* decl_ref_expr = dyn_cast< clang::DeclRefExpr >(src);
    if (decl_ref_expr == nullptr) {
        return;
    }

    auto state = m_ctx->get_state();
    SExprRef region_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(decl_ref_expr,
                                         m_ctx->get_current_location_context());

    knight_log_nl(load_expr->dumpColor(); llvm::outs() << "set load sexpr: ";
                  region_sexpr->dump(llvm::outs());
                  llvm::outs() << "\n";);

    m_ctx->set_state(state->set_stmt_sexpr(load_expr, region_sexpr));
}

void SymbolResolver::analyze_stmt(const clang::Stmt* stmt,
                                  AnalysisContext& ctx) const {
    m_ctx = &ctx;

    knight_log(stmt->printPretty(llvm::outs(),
                                 nullptr,
                                 m_ctx->get_ast_context().getPrintingPolicy());
               llvm::outs() << " \n";
               stmt->dumpColor();
               llvm::outs() << "\n";);
    if (const auto* cast_expr = llvm::dyn_cast< clang::CastExpr >(stmt)) {
        VisitCastExpr(cast_expr);
        return;
    }
    const_cast< SymbolResolver* >(this)->Visit(stmt);
}

void SymbolResolver::VisitCastExpr(const clang::CastExpr* cast_expr) const {
    auto state = m_ctx->get_state();
    const auto* src = cast_expr->getSubExpr();
    auto src_type = src->getType();
    auto dst_type = cast_expr->getType();

    bool int_cast = src_type->isIntegralOrEnumerationType() &&
                    dst_type->isIntegralOrEnumerationType();
    bool float_cast = src_type->isFloatingType() && dst_type->isFloatingType();

    if (!int_cast && !float_cast) {
        knight_log(llvm::outs() << "not int or float cast!\n";);
        return;
    }

    if (is_loading_lvalue(src)) {
        handle_load(src);
        return;
    }

    auto& ast_ctx = m_ctx->get_ast_context();
    auto src_type_size = ast_ctx.getTypeSize(src_type);
    auto dst_type_size = ast_ctx.getTypeSize(dst_type);

    SExprRef src_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(src,
                                         m_ctx->get_current_location_context());
    SExprRef dst_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(cast_expr,
                                         m_ctx->get_current_location_context());

    if (int_cast && src_type_size != dst_type_size) {
        // TODO(SymCast): add sym cast builder in symbol manager
        //  dst_expr = m_ctx->get_symbol_manager();
        if (auto y = src_sexpr->get_as_zvariable()) {
            if (auto x = dst_sexpr->get_as_zvariable()) {
                dispatch_event(
                    LinearAssignEvent(ZVarAssignZCast{static_cast< unsigned >(
                                                          dst_type_size),
                                                      dst_type,
                                                      *x,
                                                      *y},
                                      state,
                                      m_ctx));
                if (m_ctx->is_state_changed()) {
                    state = m_ctx->get_state();
                }
            }
        }
    }
    dst_sexpr = src_sexpr;

    knight_log_nl(llvm::outs() << "set cast sexpr: ";
                  dst_sexpr->dump(llvm::outs());
                  llvm::outs() << "\n";);

    m_ctx->set_state(state->set_stmt_sexpr(cast_expr, dst_sexpr));
}

} // namespace knight::dfa