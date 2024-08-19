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

#include "dfa/analysis/symbol_resolver.hpp"
#include "dfa/constraint/linear.hpp"

#define DEBUG_TYPE "SymbolResolver"

namespace knight::dfa {

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
    m_ctx->set_state(state->set_stmt_sexpr(floating_literal, scalar));
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
    if (!is_int || !is_float) {
        return;
    }

    SExprRef lhs_sexpr =
        state->get_stmt_sexpr_or_conjured(rhs_expr,
                                          m_ctx->get_current_stack_frame());

    SExprRef rhs_sexpr =
        state->get_stmt_sexpr_or_conjured(lhs_expr,
                                          m_ctx->get_current_stack_frame());

    SExprRef binary_sexpr =
        sym_mgr.get_binary_sym_expr(lhs_sexpr,
                                    rhs_sexpr,
                                    binary_operator->getOpcode(),
                                    binary_operator->getType());

    if (is_int) {
        if (auto zexpr = binary_sexpr->get_as_zexpr()) {
            ZVariable x(
                sym_mgr.get_symbol_conjured(binary_operator,
                                            m_ctx->get_current_stack_frame()));
            ZLinearExpr cstr(x);
            if (auto znum = binary_sexpr->get_as_znum()) {
                dispatch_event(
                    LinearAssignEvent(ZVarAssignZNum{x, *znum}, state, m_ctx));
                binary_sexpr =
                    sym_mgr.get_scalar_int(*znum, binary_operator->getType());
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
                state = state->add_zlinear_constraint(
                    ZLinearConstraint(cstr,
                                      LinearConstraintKind::LCK_Equality));
            }
        }
    } else if (auto qexpr = binary_sexpr->get_as_qexpr()) {
        QVariable x(
            sym_mgr.get_symbol_conjured(binary_operator,
                                        m_ctx->get_current_stack_frame()));
        QLinearExpr cstr(x);
        if (auto qnum = binary_sexpr->get_as_qnum()) {
            dispatch_event(
                LinearAssignEvent(QVarAssignQNum{x, *qnum}, state, m_ctx));
            binary_sexpr =
                sym_mgr.get_scalar_float(*qnum, binary_operator->getType());
            cstr -= *qnum;
        } else if (auto qvar = binary_sexpr->get_as_qvariable()) {
            dispatch_event(
                LinearAssignEvent(QVarAssignQVar{x, *qvar}, state, m_ctx));
            cstr -= *qvar;
        } else {
            dispatch_event(LinearAssignEvent(QVarAssignQLinearExpr{x, *qexpr},
                                             state,
                                             m_ctx));
            cstr -= *qexpr;
        }
        if (m_ctx->is_state_changed()) {
            state = m_ctx->get_state();
            state = state->add_qlinear_constraint(
                QLinearConstraint(cstr, LinearConstraintKind::LCK_Equality));
        }
    }

    state = state->set_stmt_sexpr(binary_operator, binary_sexpr);

    if (clang::BinaryOperator::isAssignmentOp(binary_operator->getOpcode())) {
        if (auto lhs_region_opt =
                state->get_region(lhs_expr, m_ctx->get_current_stack_frame())) {
            state = state->set_region_sexpr(*lhs_region_opt, rhs_sexpr);
        }
    }

    state = state->set_stmt_sexpr(binary_operator, rhs_sexpr);
    m_ctx->set_state(state);
}

void SymbolResolver::VisitDeclStmt(const clang::DeclStmt* decl_stmt) const {
    auto state = m_ctx->get_state();
    SExprRef stmt_sexpr = nullptr;
    for (const auto* decl : decl_stmt->decls()) {
        stmt_sexpr = nullptr;

        // TODO(decl-region): handle other decl types.
        if (const auto* var_decl = llvm::dyn_cast< clang::VarDecl >(decl)) {
            auto var_region_opt =
                state->get_region(var_decl, m_ctx->get_current_stack_frame());
            if (!var_region_opt.has_value()) {
                continue;
            }

            if (!var_decl->hasInit()) {
                // We don't need to handle uninitialized variables here.
                continue;
            }
            const auto* init_expr = var_decl->getInit();
            auto init_sexpr_opt = state->get_stmt_sexpr(init_expr);
            if (!init_sexpr_opt.has_value()) {
                continue;
            }

            stmt_sexpr = init_sexpr_opt.value();
            state = state->set_region_sexpr(var_region_opt.value(),
                                            init_sexpr_opt.value());
        }
    }

    if (stmt_sexpr != nullptr) {
        state = state->set_stmt_sexpr(decl_stmt, stmt_sexpr);
    }

    m_ctx->set_state(state);
}

void SymbolResolver::VisitDeclRefExpr(
    const clang::DeclRefExpr* decl_ref_expr) const {
    auto state = m_ctx->get_state();
    auto region_sexpr_opt =
        state->try_get_sexpr(decl_ref_expr,
                             m_ctx->get_current_location_context());
    if (!region_sexpr_opt.has_value()) {
        return;
    }

    LLVM_DEBUG(llvm::outs() << "declref uses region's sexpr: ";
               region_sexpr_opt.value()->dump(llvm::outs());
               llvm::outs() << "\n";);

    m_ctx->set_state(
        state->set_stmt_sexpr(decl_ref_expr, region_sexpr_opt.value()));
}

void SymbolResolver::analyze_stmt(const clang::Stmt* stmt,
                                  AnalysisContext& ctx) const {
    m_ctx = &ctx;

    LLVM_DEBUG(llvm::outs() << "SymbolResolver::pre_analyze Stmt: \n";
               stmt->dumpColor();
               llvm::outs() << "\n";);

    const_cast< SymbolResolver* >(this)->Visit(stmt);
}

} // namespace knight::dfa