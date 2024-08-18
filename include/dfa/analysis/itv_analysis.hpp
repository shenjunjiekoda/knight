//===- intervcal_analysis.hpp -----------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines an interval analysis
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis/numerical_event.hpp"
#include "dfa/analysis_context.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/domain/demo_dom.hpp"
#include "dfa/domain/interval_dom.hpp"
#include "dfa/program_state.hpp"
#include "dfa/region/region.hpp"
#include "tooling/context.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "itv_analysis"

namespace knight::dfa {

constexpr unsigned EventHandlerAlignment = 32U;
class ItvAnalysis
    : public Analysis< ItvAnalysis,
                       analyze::BeginFunction,
                       analyze::EventListener< LinearAssignEvent >,
                       analyze::PreStmt< clang::ReturnStmt > > {
  public:
    explicit ItvAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::ItvAnalysis;
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void analyze_begin_function([[maybe_unused]] AnalysisContext& ctx) const {
        LLVM_DEBUG(llvm::outs()
                       << "ItvAnalysis::analyze_begin_function()\nstate:";
                   ctx.get_state()->dump(llvm::outs());
                   llvm::outs() << "\n";);
    }

    struct EventHandler {
        const ItvAnalysis& analysis;
        ProgramStateRef state;
        AnalysisContext* ctx;

        template < typename T >
        void operator()(const T& event) const {
            this->handle(event);
        }

        void handle(const ZVarAssignZVar& var) const {
            auto zitv = state->get_clone< ZIntervalDom >();
            zitv->assign_var(var.x, var.y);
            ctx->set_state(state->set< ZIntervalDom >(zitv));
        }

        void handle(const ZVarAssignZNum& var) const {
            auto zitv = state->get_clone< ZIntervalDom >();
            zitv->assign_num(var.x, var.y);
            ctx->set_state(state->set< ZIntervalDom >(zitv));
        }

        void handle(const ZVarAssignZLinearExpr& var) const {
            auto zitv = state->get_clone< ZIntervalDom >();
            zitv->assign_linear_expr(var.x, var.y);
            ctx->set_state(state->set< ZIntervalDom >(zitv));
        }

        void handle(const QVarAssignQVar& var) const {}

        void handle(const QVarAssignQNum& var) const {}

        void handle(const QVarAssignQLinearExpr& var) const {}

    } __attribute__((aligned(EventHandlerAlignment))); // struct EventHandler

    void handle_event(const LinearAssignEvent* event) const {
        std::visit(EventHandler{*this, event->state, event->ctx},
                   event->assign);
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void pre_analyze_stmt(const clang::ReturnStmt* return_stmt,
                          AnalysisContext& ctx) const {
        LLVM_DEBUG(llvm::outs() << "ItvAnalysis::pre_analyze ReturnStmt: \n";
                   return_stmt->dumpColor();
                   llvm::outs() << "\n";);
    }

    static void add_dependencies(AnalysisManager& mgr) {
        // add dependencies here
        mgr.add_domain_dependency< ItvAnalysis, ZIntervalDom >();
    }

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        return mgr.register_analysis< ItvAnalysis >(ctx);
    }

}; // class ItvAnalysis

} // namespace knight::dfa