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
#include "dfa/analysis_context.hpp"
#include "dfa/region/region.hpp"
#include "dfa/symbol.hpp"
#include "llvm/ADT/APSInt.h"
#include "tooling/context.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtVisitor.h>
#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class SymbolResolver
    : public Analysis< SymbolResolver, analyze::EvalStmt< clang::Stmt > >,
      public clang::ConstStmtVisitor< SymbolResolver > {
  private:
    mutable AnalysisContext* m_ctx{};

  public:
    explicit SymbolResolver(KnightContext& ctx) : Analysis(ctx) {
        llvm::outs() << "SymbolResolver constructor\n";
    }

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::SymbolResolver;
    }

    void VisitIntegerLiteral(
        const clang::IntegerLiteral* integer_literal) const {
        auto state = m_ctx->get_state();
        auto ap_int = integer_literal->getValue();
        auto scalar_int =
            ScalarInt(llvm::APSInt(ap_int,
                                   integer_literal->getType()
                                       ->isUnsignedIntegerOrEnumerationType()),
                      integer_literal->getType());
        // TODO: add symbol builder
    }

    void VisitDeclRefExpr(const clang::DeclRefExpr* decl_ref_expr) const {
        auto state = m_ctx->get_state();
        if (state->get_stmt_sexpr(decl_ref_expr).has_value()) {
            return;
        }

        const auto* decl = decl_ref_expr->getDecl();
        if (decl == nullptr) {
            return;
        }

        auto region_opt =
            state->get_region(decl, m_ctx->get_current_stack_frame());
        if (!region_opt.has_value()) {
            return;
        }

        auto region_sexpr_opt = state->get_region_sexpr(region_opt.value());
        if (!region_sexpr_opt.has_value()) {
            return;
        }

        llvm::outs() << "declref uses region's sexpr: ";
        region_sexpr_opt.value()->dump(llvm::outs());
        llvm::outs() << "\n";

        m_ctx->set_state(
            state->set_stmt_sexpr(decl_ref_expr, region_sexpr_opt.value()));
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void analyze_stmt(const clang::Stmt* stmt, AnalysisContext& ctx) const {
        m_ctx = &ctx;

        llvm::outs() << "SymbolResolver::pre_analyze Stmt: \n";
        stmt->dumpColor();
        llvm::outs() << "\n";
        const_cast< SymbolResolver* >(this)->Visit(stmt);
    }

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        mgr.set_analysis_priviledged< SymbolResolver >();
        return mgr.register_analysis< SymbolResolver >(ctx);
    }
};

} // namespace knight::dfa