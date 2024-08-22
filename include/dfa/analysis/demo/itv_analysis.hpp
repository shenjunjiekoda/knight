//===- itv_analysis.hpp -----------------------------------------------===//
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
#include "dfa/analysis/core/numerical_event.hpp"
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

namespace knight::dfa {

constexpr unsigned EventHandlerAlignment = 32U;
class ItvAnalysis
    : public Analysis< ItvAnalysis,
                       analyze::BeginFunction,
                       analyze::EventListener< LinearAssignEvent >,
                       analyze::PostStmt< clang::ReturnStmt > > {
  public:
    explicit ItvAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::ItvAnalysis;
    }

    void analyze_begin_function([[maybe_unused]] AnalysisContext& ctx) const;

    struct EventHandler {
        const ItvAnalysis& analysis;
        ProgramStateRef state;
        AnalysisContext* ctx;

        template < typename T >
        void operator()(const T& event) const {
            this->handle(event);
        }

        void handle(const ZVarAssignZVar& assign) const;
        void handle(const ZVarAssignZNum& assign) const;
        void handle(const ZVarAssignZLinearExpr& assign) const;
        void handle(const ZVarAssignZCast& assign) const;
        void handle(const ZVarAssignBinaryVarVar& assign) const;
        void handle(const ZVarAssignBinaryVarNum& assign) const;
        void handle(const QVarAssignQVar& assign) const;
        void handle(const QVarAssignQNum& assign) const;
        void handle(const QVarAssignQLinearExpr& assign) const;

    } __attribute__((aligned(EventHandlerAlignment))); // struct EventHandler

    void handle_event(const LinearAssignEvent* event) const {
        std::visit(EventHandler{*this, event->state, event->ctx},
                   event->assign);
    }

    void post_analyze_stmt(const clang::ReturnStmt* return_stmt,
                           AnalysisContext& ctx) const;

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