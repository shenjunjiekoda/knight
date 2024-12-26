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

#include "analyzer/core/analysis/core/symbol_resolver.hpp"
#include "analyzer/core/analysis/core/assign_resolver.hpp"
#include "analyzer/core/analysis/core/binary_op_resolver.hpp"
#include "analyzer/core/analysis/core/numerical_event.hpp"
#include "analyzer/core/analysis/core/unary_op_resolver.hpp"
#include "analyzer/core/analysis_manager.hpp"
#include "analyzer/core/constraint/linear.hpp"
#include "analyzer/core/program_state.hpp"
#include "analyzer/core/region/region.hpp"
#include "analyzer/core/symbol.hpp"
#include "common/util/log.hpp"
#include "llvm/ADT/APInt.h"

#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>

#include <optional>

#define DEBUG_TYPE "SymbolResolver"

namespace knight::analyzer {

namespace {

bool is_loading_lvalue(const clang::Expr* expr) {
    const auto* implicit_cast_expr =
        llvm::dyn_cast_or_null< clang::ImplicitCastExpr >(expr);
    return implicit_cast_expr != nullptr &&
           implicit_cast_expr->getCastKind() == clang::CK_LValueToRValue;
}

SExprRef get_condition_sexpr(const clang::Expr* expr, AnalysisContext& ctx) {
    auto state = ctx.get_state();
    auto& sym_mgr = ctx.get_symbol_manager();
    SExprRef stmt_sexpr = nullptr;
    if (const auto* binary_operator = dyn_cast< clang::BinaryOperator >(expr);
        binary_operator != nullptr && binary_operator->isComparisonOp()) {
        auto* lhs = binary_operator->getLHS()->IgnoreParens();
        auto* rhs = binary_operator->getRHS()->IgnoreParens();

        knight_log_nl(llvm::outs() << "lhs expr: "; lhs->dumpColor();
                      llvm::outs() << "\nrhs expr: ";
                      rhs->dumpColor();
                      llvm::outs() << "\n");

        SExprRef lhs_sexpr = state->get_stmt_sexpr_or_conjured(
            lhs, ctx.get_current_location_context());
        SExprRef rhs_sexpr = state->get_stmt_sexpr_or_conjured(
            rhs, ctx.get_current_location_context());
        knight_log_nl(llvm::outs() << "lhs sexpr: " << *lhs_sexpr
                                   << "\nrhs expr: " << *rhs_sexpr << "\n"

        );

        return sym_mgr.get_binary_sym_expr(lhs_sexpr,
                                           rhs_sexpr,
                                           binary_operator->getOpcode(),
                                           binary_operator->getType());
    }
    return state
        ->get_stmt_sexpr_or_conjured(expr, ctx.get_current_location_context());
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

    m_ctx->set_state(state->set_stmt_sexpr(integer_literal,
                                           m_ctx->get_current_stack_frame(),
                                           scalar));
}

void SymbolResolver::VisitUnaryOperator(
    const clang::UnaryOperator* unary_operator) const {
    UnaryOpResolver resolver(m_ctx, this);
    resolver.resolve(unary_operator);
}

void SymbolResolver::VisitBinaryOperator(
    const clang::BinaryOperator* binary_operator) const {
    BinaryOpResolver resolver(m_ctx, this);
    resolver.resolve(binary_operator);
}

void SymbolResolver::VisitConditionalOperator(
    const clang::ConditionalOperator* conditional_operator) const {
    auto type = conditional_operator->getType();
    if (type->isIntegralOrEnumerationType()) {
        handle_int_cond_op(conditional_operator);
    }
}

void SymbolResolver::handle_int_cond_op(
    const clang::ConditionalOperator* conditional_operator) const {
    auto state = m_ctx->get_state();

    auto* true_expr = conditional_operator->getTrueExpr()->IgnoreParens();
    auto* false_expr = conditional_operator->getFalseExpr()->IgnoreParens();

    SExprRef true_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(true_expr,
                                         m_ctx->get_current_location_context());
    SExprRef false_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(false_expr,
                                         m_ctx->get_current_location_context());

    AssignResolver assign_resolver(m_ctx, this);
    ProgramStateRef state_with_true_br = assign_resolver.resolve(
        internal::AssignmentContext{std::nullopt,
                                    conditional_operator,
                                    true_expr,
                                    nullptr,
                                    true_sexpr});

    knight_log_nl(llvm::outs() << "true branch state: ";
                  state_with_true_br->dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto state_with_false_br = assign_resolver.resolve(
        internal::AssignmentContext{std::nullopt,
                                    conditional_operator,
                                    false_expr,
                                    nullptr,
                                    false_sexpr});

    knight_log_nl(llvm::outs() << "false branch state: ";
                  state_with_false_br->dump(llvm::outs());
                  llvm::outs() << "\n";);
    state = state_with_true_br->join(state_with_false_br,
                                     m_ctx->get_current_location_context());

    knight_log_nl(llvm::outs() << "conditional operator state: ";
                  state->dump(llvm::outs());
                  llvm::outs() << "\n";);

    m_ctx->set_state(state);
}

void SymbolResolver::handle_var_decl(const clang::VarDecl* var_decl,
                                     ProgramStateRef& state,
                                     SExprRef& stmt_sexpr) const {
    if (!var_decl->hasInit()) {
        // We don't need to handle uninitialized variables here.
        return;
    }
    const auto* init_expr = var_decl->getInit()->IgnoreParens();
    auto init_sexpr_opt =
        state->get_stmt_sexpr(init_expr, m_ctx->get_current_stack_frame());
    if (!init_sexpr_opt.has_value()) {
        return;
    }

    auto treg = state->get_region(var_decl, m_ctx->get_current_stack_frame());
    if (!treg) {
        llvm::WithColor::error() << "no typed region for decl!\n";
    } else {
        auto type = treg.value()->get_value_type();
        if (type->isPointerType()) {
        }
        if (type->isIntegralOrEnumerationType()) {
            AssignResolver assign_resolver(m_ctx, this);
            state = assign_resolver.resolve(
                internal::AssignmentContext{treg,
                                            std::nullopt,
                                            init_expr,
                                            nullptr,
                                            init_sexpr_opt.value()});

            init_sexpr_opt = state->get_stmt_sexpr_or_conjured(
                init_expr, m_ctx->get_current_location_context());
        }
    }
}

void SymbolResolver::VisitDeclStmt(const clang::DeclStmt* decl_stmt) const {
    auto state = m_ctx->get_state();
    SExprRef stmt_sexpr = nullptr;

    for (const auto* decl : decl_stmt->decls()) {
        stmt_sexpr = nullptr;
        // TODO(decl-region): handle other decl types.
        if (const auto* var_decl = llvm::dyn_cast< clang::VarDecl >(decl)) {
            handle_var_decl(var_decl, state, stmt_sexpr);
        }
    }

    if (stmt_sexpr != nullptr) {
        state = state->set_stmt_sexpr(decl_stmt,
                                      m_ctx->get_current_stack_frame(),
                                      stmt_sexpr);

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

    const auto* decl = decl_ref_expr->getDecl();
    auto state = m_ctx->get_state();
    auto region = state->get_region(decl, m_ctx->get_current_stack_frame());
    if (!region) {
        knight_log(llvm::outs() << "no region for decl!\n");
        return;
    }

    knight_log(llvm::outs() << "region: " << *(region.value()) << "\n");

    auto def = state->get_region_def(*region, m_ctx->get_current_stack_frame());
    if (!def) {
        knight_log(llvm::outs() << "no def for region!\n");
        return;
    }

    knight_log_nl(load_expr->dumpColor(); llvm::outs() << "set load sexpr: ";
                  def.value()->dump(llvm::outs());
                  llvm::outs() << "\n";);

    m_ctx->set_state(state->set_stmt_sexpr(load_expr,
                                           m_ctx->get_current_stack_frame(),
                                           *def));
}

void SymbolResolver::analyze_stmt(const clang::Stmt* stmt,
                                  AnalysisContext& ctx) const {
    m_ctx = &ctx;

    knight_log_nl(
        stmt->printPretty(llvm::outs(),
                          nullptr,
                          m_ctx->get_ast_context().getPrintingPolicy());
        llvm::outs() << " \n";
        stmt->getBeginLoc().dump(m_ctx->get_source_manager());
        llvm::outs() << "\n";
        stmt->dumpColor();
        llvm::outs() << "\n";);
    if (const auto* cast_expr = llvm::dyn_cast< clang::CastExpr >(stmt)) {
        VisitCastExpr(cast_expr);
        return;
    }
    const_cast< SymbolResolver* >(this)->Visit(stmt);
}

void SymbolResolver::filter_condition(const clang::Expr* condition,
                                      bool assertion_result,
                                      AnalysisContext& ctx) const {
    auto state = ctx.get_state();
    auto& sym_mgr = ctx.get_symbol_manager();
    knight_log_nl(llvm::outs() << "filter_condition: "; condition->dumpColor();
                  llvm::outs() << " " << assertion_result << "\n";
                  llvm::outs()
                  << "state before filter_condition: " << *state << "\n");
    SExprRef stmt_sexpr = get_condition_sexpr(condition, ctx);
    if (auto zvar = stmt_sexpr->get_as_zvariable()) {
        LinearNumericalAssumptionEvent
            event(PredicateZVarZNum{assertion_result ? clang::BO_NE
                                                     : clang::BO_EQ,
                                    *zvar,
                                    ZNum(0)},
                  state);
        dispatch_event(event);

    } else if (auto znum = stmt_sexpr->get_as_znum()) {
        bool is_contradiction = znum.value() == 0 && assertion_result;
        is_contradiction |= znum.value() != 0 && !assertion_result;
        if (is_contradiction) {
            state = state->set_to_bottom();
        }
    } else if (const auto* binary_sexpr =
                   llvm::dyn_cast< BinarySymExpr >(stmt_sexpr);
               binary_sexpr != nullptr && clang::BinaryOperator::isComparisonOp(
                                              binary_sexpr->get_opcode())) {
        auto op = binary_sexpr->get_opcode();
        SExprRef lhs = binary_sexpr->get_lhs();
        SExprRef rhs = binary_sexpr->get_rhs();
        auto lhs_var = lhs->get_as_zvariable();
        auto lhs_num = lhs->get_as_znum();
        auto rhs_var = rhs->get_as_zvariable();
        auto rhs_num = rhs->get_as_znum();
        if (lhs_var && rhs_var) {
            LinearNumericalAssumptionEvent
                event(PredicateZVarZVar{assertion_result
                                            ? op
                                            : clang::BinaryOperator::
                                                  negateComparisonOp(op),
                                        *lhs_var,
                                        *rhs_var},
                      state);
            dispatch_event(event);

        } else if ((lhs_var && rhs_num) || (lhs_num && rhs_var)) {
            op = assertion_result
                     ? op
                     : clang::BinaryOperator::negateComparisonOp(op);
            op = lhs_var ? op : clang::BinaryOperator::negateComparisonOp(op);
            auto l = lhs_var ? *lhs_var : *rhs_var;
            auto r = lhs_var ? *rhs_num : *lhs_num;
            LinearNumericalAssumptionEvent event(PredicateZVarZNum{op, l, r},
                                                 state);
            dispatch_event(event);
        }
    }

    const auto* bool_assertion =
        sym_mgr.get_scalar_int(ZNum(assertion_result ? 1 : 0),
                               condition->getType());
    state = state->set_stmt_sexpr(condition,
                                  ctx.get_current_stack_frame(),
                                  bool_assertion);

    knight_log(llvm::outs()
               << "state after filter_condition: " << *state << "\n");

    ctx.set_state(state);
}

void SymbolResolver::VisitCastExpr(const clang::CastExpr* cast_expr) const {
    knight_log_nl(llvm::outs() << "visit cast expr: "; cast_expr->dumpColor();
                  llvm::outs() << "\n");

    auto state = m_ctx->get_state();
    const auto* src = cast_expr->getSubExpr();
    auto src_type = src->getType();
    auto dst_type = cast_expr->getType();

    bool int_cast = src_type->isIntegralOrEnumerationType() &&
                    dst_type->isIntegralOrEnumerationType();

    if (!int_cast) {
        knight_log(llvm::outs() << "not int cast!\n";);
        return;
    }

    if (is_loading_lvalue(cast_expr)) {
        knight_log(llvm::outs() << "is loading lvalue!\n";);
        handle_load(cast_expr);
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
                LinearNumericalAssignEvent
                    event(ZVarAssignZCast{static_cast< unsigned >(
                                              dst_type_size),
                                          dst_type,
                                          *x,
                                          *y},
                          state);
                dispatch_event(event);
            }
        }
    }
    dst_sexpr = src_sexpr;

    knight_log_nl(llvm::outs() << "set cast sexpr: ";
                  dst_sexpr->dump(llvm::outs());
                  llvm::outs() << "\n";);

    m_ctx->set_state(state->set_stmt_sexpr(cast_expr,
                                           m_ctx->get_current_stack_frame(),
                                           dst_sexpr));
}

} // namespace knight::analyzer