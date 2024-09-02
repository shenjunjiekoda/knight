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
#include "dfa/region/region.hpp"
#include "dfa/symbol.hpp"
#include "llvm/ADT/APInt.h"
#include "util/log.hpp"

#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>

#include <optional>

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

    m_ctx->set_state(state->set_stmt_sexpr(integer_literal,
                                           m_ctx->get_current_stack_frame(),
                                           scalar));
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

    m_ctx->set_state(state->set_stmt_sexpr(floating_literal,
                                           m_ctx->get_current_stack_frame(),
                                           scalar));
}

void SymbolResolver::VisitUnaryOperator(
    const clang::UnaryOperator* unary_operator) const {
    auto state = m_ctx->get_state();
    auto& sym_mgr = m_ctx->get_symbol_manager();

    auto* operand_expr = unary_operator->getSubExpr();
    auto type = unary_operator->getType();
    auto& ast_ctx = m_ctx->get_ast_context();
    auto type_bit_size = ast_ctx.getTypeSize(type);

    SExprRef operand_sexpr =
        state
            ->get_stmt_sexpr_or_conjured(operand_expr,
                                         m_ctx->get_current_location_context());
    if (!type->isIntegralOrEnumerationType()) {
        knight_log(llvm::outs() << "unhandled unary operator type: " << type);
        return;
    }

    auto unary_op = unary_operator->getOpcode();
    switch (unary_op) {
        using enum clang::UnaryOperatorKind;
        case clang::UO_LNot: {
            handle_binary_operation(
                BinaryOperationContext{clang::BO_EQ,
                                       std::nullopt,
                                       sym_mgr.get_scalar_int(ZNum(0), type),
                                       operand_expr,
                                       std::nullopt,
                                       unary_operator->getType(),
                                       unary_operator});
            return;
        }
        case clang::UO_Plus:
            [[fallthrough]];
        case clang::UO_Minus: {
            handle_binary_operation(
                BinaryOperationContext{unary_op == UO_Plus ? clang::BO_Add
                                                           : clang::BO_Sub,
                                       std::nullopt,
                                       sym_mgr.get_scalar_int(ZNum(0), type),
                                       operand_expr,
                                       std::nullopt,
                                       unary_operator->getType(),
                                       unary_operator});
            return;
        } break;
        case clang::UO_PostInc:
            [[fallthrough]];
        case clang::UO_PreInc: {
            handle_binary_operation(
                BinaryOperationContext{clang::BO_AddAssign,
                                       operand_expr,
                                       std::nullopt,
                                       std::nullopt,
                                       sym_mgr.get_scalar_int(ZNum(1), type),
                                       unary_operator->getType(),
                                       unary_operator});
            return;
        }
        case clang::UO_PostDec:
            [[fallthrough]];
        case clang::UO_PreDec: {
            handle_binary_operation(
                BinaryOperationContext{clang::BO_SubAssign,
                                       operand_expr,
                                       std::nullopt,
                                       std::nullopt,
                                       sym_mgr.get_scalar_int(ZNum(1), type),
                                       unary_operator->getType(),
                                       unary_operator});
            return;
        }
        default:
            break;
    }
}

ProgramStateRef SymbolResolver::handle_assign(
    AssignmentContext assign_ctx) const {
    auto op = assign_ctx.op;
    bool is_direct_assign = !clang::BinaryOperator::isCompoundAssignmentOp(op);

    knight_assert_msg(assign_ctx.rhs_sexpr != nullptr,
                      "rhs sexpr shall be nonnull in assignment context");
    // knight_assert_msg(assign_ctx.rhs_expr != nullptr,
    //   "rhs expr shall be nonnull in assignment context");
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

    ZLinearExpr cstr;

    SymbolRef res_sym = nullptr;
    if (assign_ctx.treg) {
        res_sym =
            sym_mgr.get_region_sym_val(*assign_ctx.treg,
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

    if (assign_ctx.is_int) {
        ZVariable x(res_sym);
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
            LinearAssignEvent
                event(ZVarAssignBinaryVarVar{op, x, *lhs_var, *rhs_var}, state);
            EventDispatcher< LinearAssignEvent >::dispatch_event(event);

            cstr -= (*lhs_var + *rhs_var);
        } else if (!is_direct_assign &&
                   ((lhs_var && rhs_num) || (lhs_num && rhs_var))) {
            auto y = lhs_var ? *lhs_var : *rhs_var;
            auto z = lhs_var ? *rhs_num : *lhs_num;
            LinearAssignEvent event(ZVarAssignBinaryVarNum{op, x, y, z}, state);
            EventDispatcher< LinearAssignEvent >::dispatch_event(event);

            cstr -= (y + z);
        } else if (auto zexpr = binary_sexpr->get_as_zexpr()) {
            if (auto znum = binary_sexpr->get_as_znum()) {
                LinearAssignEvent event(ZVarAssignZNum{x, *znum}, state);
                EventDispatcher< LinearAssignEvent >::dispatch_event(event);

                binary_sexpr = sym_mgr.get_scalar_int(*znum, type);
                cstr -= *znum;
            } else if (auto zvar = binary_sexpr->get_as_zvariable()) {
                LinearAssignEvent event(ZVarAssignZVar{x, *zvar}, state);
                EventDispatcher< LinearAssignEvent >::dispatch_event(event);

                cstr -= *zvar;
            } else {
                LinearAssignEvent event(ZVarAssignZLinearExpr{x, *zexpr},
                                        state);
                EventDispatcher< LinearAssignEvent >::dispatch_event(event);

                cstr -= *zexpr;
            }
        } else if (auto rhs_expr = assign_ctx.rhs_expr) {
            ZVariable y(
                sym_mgr.get_symbol_conjured(*rhs_expr,
                                            m_ctx->get_current_stack_frame()));
            LinearAssignEvent event(ZVarAssignZVar{x, y}, state);
            EventDispatcher< LinearAssignEvent >::dispatch_event(event);
            cstr -= y;
        } else {
            knight_log(llvm::outs() << "unhandled case!\n");
            return state;
        }
        state = state->add_zlinear_constraint(
            ZLinearConstraint(cstr, LinearConstraintKind::LCK_Equality));
    } else {
        llvm::WithColor::error() << "floating point not implemented for now!\n";
    }

    if (assign_ctx.treg) {
        knight_log_nl(llvm::outs() << "\nset region: ";
                      assign_ctx.treg.value()->dump(llvm::outs());
                      llvm::outs() << " to new_def: ";
                      res_sym->dump(llvm::outs());
                      llvm::outs() << "\n");

        state = state->set_region_def(*assign_ctx.treg,
                                      m_ctx->get_current_stack_frame(),
                                      cast< RegionSymVal >(res_sym));
    } else {
        state = state->set_stmt_sexpr(*assign_ctx.stmt,
                                      m_ctx->get_current_stack_frame(),
                                      res_sym);
    }

    return state;
}

void SymbolResolver::VisitBinaryOperator(
    const clang::BinaryOperator* binary_operator) const {
    handle_binary_operation(BinaryOperationContext{binary_operator->getOpcode(),
                                                   binary_operator->getLHS(),
                                                   std::nullopt,
                                                   binary_operator->getRHS(),
                                                   std::nullopt,
                                                   binary_operator->getType(),
                                                   binary_operator});
}

void SymbolResolver::handle_binary_operation(
    BinaryOperationContext bo_ctx) const {
    using enum clang::BinaryOperator::Opcode;

    auto state = m_ctx->get_state();
    auto& sym_mgr = m_ctx->get_symbol_manager();

    auto lhs_expr = bo_ctx.lhs_expr;
    auto rhs_expr = bo_ctx.rhs_expr;

    bool is_int = bo_ctx.result_type->isIntegralOrEnumerationType();
    bool is_float = bo_ctx.result_type->isFloatingType();
    if (!is_int && !is_float) {
        knight_log(llvm::outs() << "not int or float!\n");
        return;
    }
    auto op = bo_ctx.op;
    bool is_assign = clang::BinaryOperator::isAssignmentOp(op);
    bool is_direct_assign =
        is_assign && !clang::BinaryOperator::isCompoundAssignmentOp(op);
    SExprRef lhs_sexpr = nullptr;
    if (is_assign) {
        if (is_direct_assign) {
            lhs_sexpr =
                sym_mgr.get_symbol_conjured(*lhs_expr,
                                            m_ctx->get_current_stack_frame());
        } else {
            lhs_sexpr = state
                            ->get_stmt_sexpr(*lhs_expr,
                                             m_ctx->get_current_stack_frame())
                            .value_or(nullptr);
        }
    } else {
        if (lhs_expr) {
            lhs_sexpr = state->get_stmt_sexpr_or_conjured(
                *lhs_expr, m_ctx->get_current_location_context());
        } else {
            lhs_sexpr = *bo_ctx.lhs_sexpr;
        }
    }

    if (lhs_sexpr == nullptr) {
        knight_log(llvm::outs() << "lhs_sexpr is null!\n");
        return;
    }

    SExprRef rhs_sexpr =
        rhs_expr ? state->get_stmt_sexpr_or_conjured(
                       *rhs_expr, m_ctx->get_current_location_context())
                 : *bo_ctx.rhs_sexpr;

    knight_log_nl(llvm::outs() << "lhs_sexpr: "; lhs_sexpr->dump(llvm::outs());
                  llvm::outs() << "\nrhs_sexpr: ";
                  rhs_sexpr->dump(llvm::outs());
                  llvm::outs() << "\n";);

    if (is_assign) {
        auto treg =
            state->get_region(*lhs_expr, m_ctx->get_current_stack_frame());
        std::optional< const clang::Stmt* > stmt =
            treg ? std::nullopt : std::make_optional(bo_ctx.result_stmt);
        m_ctx->set_state(handle_assign(AssignmentContext{is_int,
                                                         treg,
                                                         stmt,
                                                         rhs_expr,
                                                         lhs_sexpr,
                                                         rhs_sexpr,
                                                         op}));
        return;
    }

    SExprRef binary_sexpr = sym_mgr.get_binary_sym_expr(lhs_sexpr,
                                                        rhs_sexpr,
                                                        bo_ctx.op,
                                                        bo_ctx.result_type);
    knight_log(llvm::outs() << "binary_sexpr: " << *binary_sexpr << "\n");

    const auto* binary_conjured_sym =
        sym_mgr.get_symbol_conjured(bo_ctx.result_stmt,
                                    bo_ctx.result_type,
                                    m_ctx->get_current_stack_frame());

    if (is_int) {
        ZVariable x(binary_conjured_sym);

        knight_log_nl(llvm::outs() << "zvar x: "; x.dump(llvm::outs());
                      llvm::outs() << "\n";);

        ZLinearExpr cstr(x);
        auto lhs_var = lhs_sexpr->get_as_zvariable();
        auto lhs_num = lhs_sexpr->get_as_znum();
        auto rhs_var = rhs_sexpr->get_as_zvariable();
        auto rhs_num = rhs_sexpr->get_as_znum();
        if (lhs_var && rhs_var) {
            LinearAssignEvent
                event(ZVarAssignBinaryVarVar{op, x, *lhs_var, *rhs_var}, state);
            EventDispatcher< LinearAssignEvent >::dispatch_event(event);

            cstr -= (*lhs_var + *rhs_var);
        } else if ((lhs_var && rhs_num) || (lhs_num && rhs_var)) {
            auto y = lhs_var ? *lhs_var : *rhs_var;
            auto z = lhs_var ? *rhs_num : *lhs_num;
            LinearAssignEvent event(ZVarAssignBinaryVarNum{op, x, y, z}, state);
            EventDispatcher< LinearAssignEvent >::dispatch_event(event);

            cstr -= (y + z);
        } else if (auto zexpr = binary_sexpr->get_as_zexpr()) {
            if (auto znum = binary_sexpr->get_as_znum()) {
                LinearAssignEvent event(ZVarAssignZNum{x, *znum}, state);
                EventDispatcher< LinearAssignEvent >::dispatch_event(event);

                binary_sexpr =
                    sym_mgr.get_scalar_int(*znum, bo_ctx.result_type);
                cstr -= *znum;
            } else if (auto zvar = binary_sexpr->get_as_zvariable()) {
                LinearAssignEvent event(ZVarAssignZVar{x, *zvar}, state);
                EventDispatcher< LinearAssignEvent >::dispatch_event(event);

                cstr -= *zvar;
            } else {
                LinearAssignEvent event(ZVarAssignZLinearExpr{x, *zexpr},
                                        state);
                EventDispatcher< LinearAssignEvent >::dispatch_event(event);

                cstr -= *zexpr;
            }
            state = state->add_zlinear_constraint(
                ZLinearConstraint(cstr, LinearConstraintKind::LCK_Equality));
        }
        // binary_sexpr = ;
        // TODO(floating-bo): support fp
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

void SymbolResolver::VisitConditionalOperator(
    const clang::ConditionalOperator* conditional_operator) const {
    auto type = conditional_operator->getType();
    if (!type->isIntegralOrEnumerationType()) {
        return;
    }
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

    auto state_with_true_br =
        handle_assign(AssignmentContext{true,
                                        std::nullopt,
                                        conditional_operator,
                                        true_expr,
                                        nullptr,
                                        true_sexpr});

    knight_log_nl(llvm::outs() << "true branch state: ";
                  state_with_true_br->dump(llvm::outs());
                  llvm::outs() << "\n";);

    auto state_with_false_br =
        handle_assign(AssignmentContext{true,
                                        std::nullopt,
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
            const auto* init_expr = var_decl->getInit()->IgnoreParens();
            auto init_sexpr_opt =
                state->get_stmt_sexpr(init_expr,
                                      m_ctx->get_current_stack_frame());
            if (!init_sexpr_opt.has_value()) {
                continue;
            }

            if (auto treg =
                    state->get_region(var_decl,
                                      m_ctx->get_current_stack_frame())) {
                auto type = treg.value()->get_value_type();
                state = handle_assign(
                    AssignmentContext{type->isIntegralOrEnumerationType(),
                                      treg,
                                      std::nullopt,
                                      init_expr,
                                      nullptr,
                                      init_sexpr_opt.value()});

                init_sexpr_opt = state->get_stmt_sexpr_or_conjured(
                    init_expr, m_ctx->get_current_location_context());

            } else {
                llvm::WithColor::error() << "no typed region for decl!\n";
            }
            stmt_sexpr = *init_sexpr_opt;
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

void SymbolResolver::filter_condition(const clang::Expr* expr,
                                      bool assertion_result,
                                      AnalysisContext& ctx) const {
    auto state = ctx.get_state();
    auto& sym_mgr = ctx.get_symbol_manager();
    knight_log_nl(llvm::outs() << "filter_condition: "; expr->dumpColor();
                  llvm::outs() << " " << assertion_result << "\n";
                  llvm::outs()
                  << "state before filter_condition: " << *state << "\n");

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

        stmt_sexpr = sym_mgr.get_binary_sym_expr(lhs_sexpr,
                                                 rhs_sexpr,
                                                 binary_operator->getOpcode(),
                                                 binary_operator->getType());
    } else {
        stmt_sexpr = state->get_stmt_sexpr_or_conjured(
            expr, ctx.get_current_location_context());
    }

    if (auto zvar = stmt_sexpr->get_as_zvariable()) {
        LinearAssumptionEvent event(PredicateZVarZNum{assertion_result
                                                          ? clang::BO_NE
                                                          : clang::BO_EQ,
                                                      *zvar,
                                                      ZNum(0)},
                                    state);
        EventDispatcher< LinearAssumptionEvent >::dispatch_event(event);

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
            LinearAssumptionEvent
                event(PredicateZVarZVar{assertion_result
                                            ? op
                                            : clang::BinaryOperator::
                                                  negateComparisonOp(op),
                                        *lhs_var,
                                        *rhs_var},
                      state);
            EventDispatcher< LinearAssumptionEvent >::dispatch_event(event);

        } else if ((lhs_var && rhs_num) || (lhs_num && rhs_var)) {
            op = assertion_result
                     ? op
                     : clang::BinaryOperator::negateComparisonOp(op);
            op = lhs_var ? op : clang::BinaryOperator::negateComparisonOp(op);
            auto l = lhs_var ? *lhs_var : *rhs_var;
            auto r = lhs_var ? *rhs_num : *lhs_num;
            LinearAssumptionEvent event(PredicateZVarZNum{op, l, r}, state);
            EventDispatcher< LinearAssumptionEvent >::dispatch_event(event);
        }
    }

    const auto* bool_assertion =
        sym_mgr.get_scalar_int(ZNum(assertion_result ? 1 : 0), expr->getType());
    state = state->set_stmt_sexpr(expr,
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
    bool float_cast = src_type->isFloatingType() && dst_type->isFloatingType();

    if (!int_cast && !float_cast) {
        knight_log(llvm::outs() << "not int or float cast!\n";);
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
                LinearAssignEvent event(ZVarAssignZCast{static_cast< unsigned >(
                                                            dst_type_size),
                                                        dst_type,
                                                        *x,
                                                        *y},
                                        state);
                EventDispatcher< LinearAssignEvent >::dispatch_event(event);
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

} // namespace knight::dfa