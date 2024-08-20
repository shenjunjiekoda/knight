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

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtVisitor.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class SymbolResolver
    : public Analysis< SymbolResolver,
                       analyze::EvalStmt< clang::Stmt >,
                       analyze::EventDispatcher< LinearAssignEvent > >,
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

    void VisitFloatingLiteral(
        const clang::FloatingLiteral* floating_literal) const;

    void VisitBinaryOperator(
        const clang::BinaryOperator* binary_operator) const;

    void VisitDeclStmt(const clang::DeclStmt* decl_stmt) const;

    void VisitDeclRefExpr(const clang::DeclRefExpr* decl_ref_expr) const;

    void VisitCastExpr(const clang::CastExpr* cast_expr) const;

    void analyze_stmt(const clang::Stmt* stmt, AnalysisContext& ctx) const;

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        mgr.set_analysis_privileged< SymbolResolver >();
        return mgr.register_analysis< SymbolResolver >(ctx);
    }
};

} // namespace knight::dfa