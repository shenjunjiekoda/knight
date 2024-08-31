//===- numerical_analysis.hpp -----------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines an numerical analysis
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis/analyses.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis/core/numerical_event.hpp"
#include "dfa/analysis_context.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/domain/demo_dom.hpp"
#include "dfa/domain/domains.hpp"
#include "dfa/domain/numerical/interval_dom.hpp"
#include "dfa/program_state.hpp"
#include "dfa/region/region.hpp"
#include "tooling/context.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <llvm/Support/raw_ostream.h>

#include <utility>
#include "util/log.hpp"

namespace knight::dfa {

constexpr unsigned EventHandlerAlignment = 16U;
class NumericalAnalysis
    : public Analysis< NumericalAnalysis,
                       analyze::EventListener< LinearAssignEvent >,
                       analyze::EventListener< LinearAssumptionEvent > > {
  public:
    explicit NumericalAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::NumericalAnalysis;
    }

    struct LinearAssignEventHandler {
        const NumericalAnalysis& analysis;
        ProgramStateRef& state;

        LinearAssignEventHandler(const NumericalAnalysis& analysis,
                                 ProgramStateRef& state)
            : analysis(analysis), state(state) {}

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

    void handle_event(LinearAssignEvent* event) const {
        llvm::outs() << "before handle event state: " << *(event->state)
                     << "\n";
        std::visit(LinearAssignEventHandler{*this, event->state},
                   event->assign);
        llvm::outs() << "after handle event state: " << *(event->state) << "\n";
    }

    struct LinearAssumptionEventHandler {
        const NumericalAnalysis& analysis;
        ProgramStateRef& state;

        LinearAssumptionEventHandler(const NumericalAnalysis& analysis,
                                     ProgramStateRef& state)
            : analysis(analysis), state(state) {}

        template < typename T >
        void operator()(const T& event) const {
            this->handle(event);
        }

        void handle(const PredicateZVarZNum& pred) const;
        void handle(const PredicateZVarZVar& pred) const;
        void handle(const GeneralLinearConstraint& cstr) const;

    } __attribute__((
        aligned(EventHandlerAlignment))); // struct LinearAssumptionEventHandler

    void handle_event(LinearAssumptionEvent* event) const {
        std::visit(LinearAssumptionEventHandler{*this, event->state},
                   event->assumption);
    }

    static void add_dependencies(AnalysisManager& mgr) {
        auto zdom_id =
            get_domain_id(mgr.get_context().get_current_options().zdom);
        mgr.add_domain_dependency(get_analysis_id(get_kind()), zdom_id);
        mgr.add_domain_dependency< NumericalAnalysis, ZIntervalDom >();
    }

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        return mgr.register_analysis< NumericalAnalysis >(ctx);
    }

}; // class NumericalAnalysis

} // namespace knight::dfa