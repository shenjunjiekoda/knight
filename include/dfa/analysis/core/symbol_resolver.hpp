//===- symbol_resolver.hpp --------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the symbol resolver.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis/core/numerical_event.hpp"
#include "dfa/analysis/core/pointer_event.hpp"
#include "dfa/analysis_context.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/region/region.hpp"
#include "dfa/symbol.hpp"
#include "dfa/symbol_manager.hpp"
#include "tooling/context.hpp"
#include "util/log.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/AST/Type.h>
#include <llvm/ADT/APSInt.h>

namespace knight::dfa {

namespace internal {

constexpr unsigned BinaryOperationContextAlignment = 128U;
constexpr unsigned AssignmentContextAlignment = 64U;

} // namespace internal

class SymbolResolver
    : public Analysis< SymbolResolver,
                       analyze::EvalStmt< clang::Stmt >,
                       analyze::EventDispatcher< LinearNumericalAssignEvent,
                                                 LinearNumericalAssumptionEvent,
                                                 PointerAssignEvent >,
                       analyze::ConditionFilter >,
      public clang::ConstStmtVisitor< SymbolResolver > {
  private:
    mutable AnalysisContext* m_ctx{};

  public:
    explicit SymbolResolver(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::SymbolResolver;
    }

    void VisitIntegerLiteral(const clang::IntegerLiteral*) const;
    void VisitUnaryOperator(const clang::UnaryOperator*) const;
    void VisitBinaryOperator(const clang::BinaryOperator*) const;
    void VisitConditionalOperator(const clang::ConditionalOperator*) const;
    void VisitDeclStmt(const clang::DeclStmt*) const;
    void VisitCastExpr(const clang::CastExpr*) const;

    void analyze_stmt(const clang::Stmt*, AnalysisContext&) const;
    void filter_condition(const clang::Expr*, bool, AnalysisContext&) const;

    [[nodiscard]] static UniqueAnalysisRef register_analysis(
        AnalysisManager& mgr, KnightContext& ctx) {
        mgr.set_analysis_privileged< SymbolResolver >();
        return mgr.register_analysis< SymbolResolver >(ctx);
    }

  private:
    struct alignas(internal::BinaryOperationContextAlignment)
        BinaryOperationContext {
        clang::BinaryOperator::Opcode op{};
        // lhs_expr has value or lhs_sexpr has value
        std::optional< const clang::Expr* > lhs_expr;
        std::optional< SExprRef > lhs_sexpr;

        // rhs_expr has value or rhs_sexpr has value
        std::optional< const clang::Expr* > rhs_expr;
        std::optional< SExprRef > rhs_sexpr;

        clang::QualType result_type;
        const clang::Stmt* result_stmt{};
    }; // struct BinaryOperationContext

    void handle_var_decl(const clang::VarDecl* var_decl,
                         ProgramStateRef& state,
                         SExprRef& stmt_sexpr) const;

    void handle_int_unary_operation(const clang::UnaryOperator*) const;
    void handle_ptr_unary_operation(const clang::UnaryOperator*) const;

    void handle_binary_operation(BinaryOperationContext) const;
    void handle_assign_binary_operation(BinaryOperationContext) const;
    void handle_int_binary_operation(BinaryOperationContext) const;
    void handle_int_non_assign_binary_operation(BinaryOperationContext) const;
    void handle_ref_binary_operation(BinaryOperationContext) const;
    void handle_ptr_binary_operation(BinaryOperationContext) const;

    void handle_int_cond_op(const clang::ConditionalOperator*) const;

    struct alignas(internal::AssignmentContextAlignment)
        AssignmentContext { // NOLINT(altera-struct-pack-align)
        std::optional< const TypedRegion* > treg = std::nullopt;
        std::optional< const clang::Stmt* > stmt = std::nullopt;
        std::optional< const clang::Expr* > rhs_expr = std::nullopt;
        SExprRef lhs_sexpr{};
        SExprRef rhs_sexpr{};
        clang::BinaryOperator::Opcode op =
            clang::BinaryOperator::Opcode::BO_Assign;
    };

    [[nodiscard]] ProgramStateRef handle_assign(AssignmentContext) const;
    void handle_int_assign(AssignmentContext assign_ctx,
                           SymbolRef res_sym,
                           bool is_direct_assign,
                           clang::BinaryOperator::Opcode op,
                           ProgramStateRef& state,
                           SExprRef& binary_sexpr) const;
    void handle_ptr_assign(AssignmentContext assign_ctx,
                           SymbolRef res_sym,
                           bool is_direct_assign,
                           clang::BinaryOperator::Opcode op,
                           ProgramStateRef& state,
                           SExprRef& binary_sexpr) const;

    void handle_load(const clang::Expr* load_expr) const;
};

} // namespace knight::dfa