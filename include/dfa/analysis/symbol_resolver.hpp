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
#include "dfa/symbol_manager.hpp"
#include "tooling/context.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtVisitor.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "symbol-resolver"

namespace knight::dfa {

class SymbolResolver
    : public Analysis< SymbolResolver, analyze::EvalStmt< clang::Stmt > >,
      public clang::ConstStmtVisitor< SymbolResolver > {
  private:
    mutable AnalysisContext* m_ctx{};

  public:
    explicit SymbolResolver(KnightContext& ctx) : Analysis(ctx) {
        LLVM_DEBUG(llvm::outs() << "SymbolResolver constructor\n");
    }

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::SymbolResolver;
    }

    void VisitIntegerLiteral(
        const clang::IntegerLiteral* integer_literal) const {
        auto state = m_ctx->get_state();
        auto ap_int = integer_literal->getValue();
        auto aps_int = llvm::APSInt(ap_int,
                                    integer_literal->getType()
                                        ->isUnsignedIntegerOrEnumerationType());

        SymbolManager& symbol_manager = m_ctx->get_symbol_manager();
        const auto* scalar =
            symbol_manager.get_scalar_int(aps_int, integer_literal->getType());
        m_ctx->set_state(state->set_stmt_sexpr(integer_literal, scalar));
    }

    void VisitFloatingLiteral(
        const clang::FloatingLiteral* floating_literal) const {
        auto state = m_ctx->get_state();
        auto ap_float = floating_literal->getValue();
        auto aps_float = llvm::APFloat(ap_float);

        SymbolManager& symbol_manager = m_ctx->get_symbol_manager();
        const auto* scalar =
            symbol_manager.get_scalar_float(aps_float,
                                            floating_literal->getType());
        m_ctx->set_state(state->set_stmt_sexpr(floating_literal, scalar));
    }

    void VisitBinaryOperator(
        const clang::BinaryOperator* binary_operator) const {
        using enum clang::BinaryOperator::Opcode;
        auto state = m_ctx->get_state();
        auto* lhs = binary_operator->getLHS();
        auto* rhs = binary_operator->getRHS();
        auto& sym_mgr = m_ctx->get_symbol_manager();

        SExprRef rhs_sexpr =
            state->get_stmt_sexpr_or_conjured(rhs,
                                              m_ctx->get_current_stack_frame(),
                                              sym_mgr);

        switch (binary_operator->getOpcode()) {
            case clang::BO_Assign: {
                if (auto region_opt =
                        state->get_region(lhs,
                                          m_ctx->get_current_stack_frame())) {
                    state =
                        state->set_region_sexpr(region_opt.value(), rhs_sexpr);
                }
                state = state->set_stmt_sexpr(binary_operator, rhs_sexpr);
            } break;
            case clang::BO_Add:
                [[fallthrough]];
            case clang::BO_Sub:
                [[fallthrough]];
            case clang::BO_Mul:
                [[fallthrough]];
            case clang::BO_Div:
                [[fallthrough]];
            case clang::BO_Rem:
                [[fallthrough]];
            case clang::BO_Shl:
                [[fallthrough]];
            case clang::BO_Shr:
                [[fallthrough]];
            case clang::BO_LT:
                [[fallthrough]];
            case clang::BO_GT:
                [[fallthrough]];
            case clang::BO_LE:
                [[fallthrough]];
            case clang::BO_GE:
                [[fallthrough]];
            case clang::BO_EQ:
                [[fallthrough]];
            case clang::BO_NE: {
                SExprRef lhs_sexpr = state->get_stmt_sexpr_or_conjured(
                    lhs,
                    m_ctx->get_current_stack_frame(),
                    m_ctx->get_symbol_manager());
                SExprRef res = sym_mgr.normalize(
                    sym_mgr.get_binary_sym_expr(lhs_sexpr,
                                                rhs_sexpr,
                                                binary_operator->getOpcode(),
                                                binary_operator->getType()));
                state = state->set_stmt_sexpr(binary_operator, res);
            } break;
            default:
                knight_unreachable("unhanlded binary operator kind"); // NOLINT
                break;
        }
        m_ctx->set_state(state);
    }

    void VisitDeclStmt(const clang::DeclStmt* decl_stmt) const {
        auto state = m_ctx->get_state();
        SExprRef stmt_sexpr = nullptr;
        for (const auto* decl : decl_stmt->decls()) {
            stmt_sexpr = nullptr;

            // TODO(decl-region): handle other decl types.
            if (const auto* var_decl = llvm::dyn_cast< clang::VarDecl >(decl)) {
                auto var_region_opt =
                    state->get_region(var_decl,
                                      m_ctx->get_current_stack_frame());
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

    void VisitDeclRefExpr(const clang::DeclRefExpr* decl_ref_expr) const {
        auto state = m_ctx->get_state();
        auto region_sexpr_opt =
            state->try_get_sexpr(decl_ref_expr,
                                 m_ctx->get_current_location_context(),
                                 m_ctx->get_symbol_manager());
        if (!region_sexpr_opt.has_value()) {
            return;
        }

        LLVM_DEBUG(llvm::outs() << "declref uses region's sexpr: ";
                   region_sexpr_opt.value()->dump(llvm::outs());
                   llvm::outs() << "\n";);

        m_ctx->set_state(
            state->set_stmt_sexpr(decl_ref_expr, region_sexpr_opt.value()));
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void analyze_stmt(const clang::Stmt* stmt, AnalysisContext& ctx) const {
        m_ctx = &ctx;

        LLVM_DEBUG(llvm::outs() << "SymbolResolver::pre_analyze Stmt: \n";
                   stmt->dumpColor();
                   llvm::outs() << "\n";);

        const_cast< SymbolResolver* >(this)->Visit(stmt);
    }

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        mgr.set_analysis_priviledged< SymbolResolver >();
        return mgr.register_analysis< SymbolResolver >(ctx);
    }
};

} // namespace knight::dfa