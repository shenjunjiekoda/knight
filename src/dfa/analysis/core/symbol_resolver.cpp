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
#include "dfa/analysis/core/numerical_event.hpp"
#include "dfa/constraint/linear.hpp"
#include "llvm/Support/raw_ostream.h"

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

    LLVM_DEBUG(llvm::outs() << "SymbolResolver::VisitIntegerLiteral: "
                            << "set integer literal: ";
               integer_literal
                   ->printPretty(llvm::outs(),
                                 nullptr,
                                 m_ctx->get_ast_context().getPrintingPolicy());
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

    LLVM_DEBUG(llvm::outs() << "SymbolResolver::VisitFloatLiteral: "
                            << "set floating literal: ";
               floating_literal
                   ->printPretty(llvm::outs(),
                                 nullptr,
                                 m_ctx->get_ast_context().getPrintingPolicy());
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

    SExprRef operand_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(operand_expr,
                                         m_ctx->get_current_location_context());
    if (auto y_var = operand_sexpr->get_as_zvariable()) {
        ZVariable x(
            sym_mgr.get_symbol_conjured(unary_operator,
                                        m_ctx->get_current_stack_frame()));
        // dispatch_event() ;
    }
    SExprRef unary_sexpr =
        sym_mgr.get_unary_sym_expr(operand_sexpr,
                                   unary_operator->getOpcode(),
                                   unary_operator->getType());

    LLVM_DEBUG(llvm::outs() << "SymbolResolver::VisitUnaryOperator: "
                            << "set unary operator: ";
               unary_operator
                   ->printPretty(llvm::outs(),
                                 nullptr,
                                 m_ctx->get_ast_context().getPrintingPolicy());
               llvm::outs() << " to sexpr: ";
               unary_sexpr->dump(llvm::outs());
               llvm::outs() << "\n";);

    state = state->set_stmt_sexpr(unary_operator, unary_sexpr);
    m_ctx->set_state(state);
}

void SymbolResolver::VisitBinaryOperator(
    const clang::BinaryOperator* binary_operator) const {
    using enum clang::BinaryOperator::Opcode;

    LLVM_DEBUG(llvm::dbgs() << "SymbolResolver::VisitBinaryOperator: "
                            << binary_operator->getOpcodeStr() << "\n");

    auto state = m_ctx->get_state();
    auto& sym_mgr = m_ctx->get_symbol_manager();

    auto* lhs_expr = binary_operator->getLHS();
    auto* rhs_expr = binary_operator->getRHS();

    bool is_int = binary_operator->getType()->isIntegralOrEnumerationType();
    bool is_float = binary_operator->getType()->isFloatingType();
    if (!is_int && !is_float) {
        LLVM_DEBUG(llvm::outs() << "not int or float!\n");
        return;
    }

    SExprRef lhs_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(lhs_expr,
                                         m_ctx->get_current_location_context());

    SExprRef rhs_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(rhs_expr,
                                         m_ctx->get_current_location_context());

    SExprRef binary_sexpr =
        sym_mgr.get_binary_sym_expr(lhs_sexpr,
                                    rhs_sexpr,
                                    binary_operator->getOpcode(),
                                    binary_operator->getType());

    LLVM_DEBUG(llvm::outs() << "lhs_sexpr: "; lhs_sexpr->dump(llvm::outs());
               llvm::outs() << "\nrhs_sexpr: ";
               rhs_sexpr->dump(llvm::outs());
               llvm::outs() << "\n";);

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
            }
            state = state->add_zlinear_constraint(
                ZLinearConstraint(cstr, LinearConstraintKind::LCK_Equality));
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
        }
        state = state->add_qlinear_constraint(
            QLinearConstraint(cstr, LinearConstraintKind::LCK_Equality));
    }

    state = state->set_stmt_sexpr(binary_operator, binary_sexpr);

    LLVM_DEBUG(llvm::outs() << "set binary_sexpr: ";
               binary_sexpr->dump(llvm::outs());
               llvm::outs() << "\n";);

    if (clang::BinaryOperator::isAssignmentOp(binary_operator->getOpcode())) {
        if (auto lhs_region_opt =
                state->get_region(lhs_expr, m_ctx->get_current_stack_frame())) {
            LLVM_DEBUG(llvm::outs() << "\n set lhs region: ";
                       lhs_region_opt.value()->dump(llvm::outs());
                       llvm::outs() << " to binary_sexpr: ";
                       rhs_sexpr->dump(llvm::outs());
                       llvm::outs() << "\n");

            state = state->set_region_sexpr(*lhs_region_opt, binary_sexpr);
        }
    }

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

            stmt_sexpr = *init_sexpr_opt;

            if (auto var_region =
                    state->get_region(var_decl,
                                      m_ctx->get_current_stack_frame())) {
                state = state->set_region_sexpr(*var_region,
                                                init_sexpr_opt.value());

                LLVM_DEBUG(llvm::outs() << "set var region sexpr: ";
                           var_region.value()->dump(llvm::outs());
                           llvm::outs() << " to: ";
                           init_sexpr_opt.value()->dump(llvm::outs());
                           llvm::outs() << "\n";);
            }
        }
    }

    if (stmt_sexpr != nullptr) {
        state = state->set_stmt_sexpr(decl_stmt, stmt_sexpr);

        LLVM_DEBUG(llvm::outs() << "set decl_stmt sexpr: ";
                   llvm::outs() << " to: ";
                   stmt_sexpr->dump(llvm::outs());
                   llvm::outs() << "\n";);
    }

    m_ctx->set_state(state);
}

void SymbolResolver::VisitDeclRefExpr(
    const clang::DeclRefExpr* decl_ref_expr) const {
    auto state = m_ctx->get_state();
    SExprRef region_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(decl_ref_expr,
                                         m_ctx->get_current_location_context());

    LLVM_DEBUG(llvm::outs() << "set declref sexpr: ";
               region_sexpr->dump(llvm::outs());
               llvm::outs() << "\n";);

    m_ctx->set_state(state->set_stmt_sexpr(decl_ref_expr, region_sexpr));
}

void SymbolResolver::analyze_stmt(const clang::Stmt* stmt,
                                  AnalysisContext& ctx) const {
    m_ctx = &ctx;

    LLVM_DEBUG(llvm::outs() << "SymbolResolver::pre_analyze Stmt:";
               stmt->printPretty(llvm::outs(),
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
    LLVM_DEBUG(
        llvm::outs() << "SymbolResolver::VisitCastExpr: ";
        cast_expr->printPretty(llvm::outs(),
                               nullptr,
                               m_ctx->get_ast_context().getPrintingPolicy());
        llvm::outs() << "\n";);

    auto state = m_ctx->get_state();
    const auto* src = cast_expr->getSubExpr();
    auto src_type = src->getType();
    auto dst_type = cast_expr->getType();

    bool int_cast = src_type->isIntegralOrEnumerationType() &&
                    dst_type->isIntegralOrEnumerationType();
    bool float_cast = src_type->isFloatingType() && dst_type->isFloatingType();

    if (!int_cast && !float_cast) {
        llvm::outs() << "not int or float cast!\n";
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

    LLVM_DEBUG(llvm::outs() << "set cast sexpr: ";
               dst_sexpr->dump(llvm::outs());
               llvm::outs() << "\n";);

    m_ctx->set_state(state->set_stmt_sexpr(cast_expr, dst_sexpr));
}

} // namespace knight::dfa