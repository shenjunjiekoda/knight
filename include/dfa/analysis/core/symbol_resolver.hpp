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
    void handle_var_decl(const clang::VarDecl* var_decl,
                         ProgramStateRef& state,
                         SExprRef& stmt_sexpr) const;

    void handle_int_cond_op(const clang::ConditionalOperator*) const;

    void handle_load(const clang::Expr* load_expr) const;
};

} // namespace knight::dfa