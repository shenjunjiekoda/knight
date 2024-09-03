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

constexpr unsigned AssignmentContextAlignment = 64U;

} // namespace internal

class SymbolResolver
    : public Analysis< SymbolResolver,
                       analyze::EvalStmt< clang::Stmt >,
                       analyze::EventDispatcher< LinearAssignEvent >,
                       analyze::EventDispatcher< LinearAssumptionEvent >,
                       analyze::ConditionFilter >,
      public clang::ConstStmtVisitor< SymbolResolver > {
  private:
    mutable AnalysisContext* m_ctx{};

  public:
    explicit SymbolResolver(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::SymbolResolver;
    }

    void VisitIntegerLiteral(
        const clang::IntegerLiteral* integer_literal) const;

    void VisitUnaryOperator(const clang::UnaryOperator* unary_operator) const;

    void VisitBinaryOperator(
        const clang::BinaryOperator* binary_operator) const;

    void VisitConditionalOperator(
        const clang::ConditionalOperator* conditional_operator) const;

    void VisitDeclStmt(const clang::DeclStmt* decl_stmt) const;

    void VisitCastExpr(const clang::CastExpr* cast_expr) const;

    void analyze_stmt(const clang::Stmt* stmt, AnalysisContext& ctx) const;

    void filter_condition(const clang::Expr* expr,
                          bool assertion_result,
                          AnalysisContext& ctx) const;

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        mgr.set_analysis_privileged< SymbolResolver >();
        return mgr.register_analysis< SymbolResolver >(ctx);
    }

  private:
    struct BinaryOperationContext {
        clang::BinaryOperator::Opcode op;
        // lhs_expr has value or lhs_sexpr has value
        std::optional< const clang::Expr* > lhs_expr;
        std::optional< SExprRef > lhs_sexpr;

        // rhs_expr has value or rhs_sexpr has value
        std::optional< const clang::Expr* > rhs_expr;
        std::optional< SExprRef > rhs_sexpr;

        clang::QualType result_type;
        const clang::Stmt* result_stmt;
    }; // struct BinaryOperationContext

    void handle_binary_operation(BinaryOperationContext bo_ctx) const;

    struct AssignmentContext {
        std::optional< const TypedRegion* > treg = std::nullopt;
        std::optional< const clang::Stmt* > stmt = std::nullopt;
        std::optional< const clang::Expr* > rhs_expr = std::nullopt;
        SExprRef lhs_sexpr{};
        SExprRef rhs_sexpr{};
        clang::BinaryOperator::Opcode op =
            clang::BinaryOperator::Opcode::BO_Assign;
    } __attribute__((aligned(internal::AssignmentContextAlignment)))
    __attribute__((packed));

    ProgramStateRef handle_assign(AssignmentContext assign_ctx) const;
    void handle_load(const clang::Expr* load_expr) const;
};

} // namespace knight::dfa