//===- numerical_analysis.hpp -----------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the numerical analysis
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/core/analysis/analyses.hpp"
#include "analyzer/core/analysis/analysis_base.hpp"
#include "analyzer/core/analysis/core/numerical_event.hpp"
#include "analyzer/core/analysis_context.hpp"
#include "analyzer/core/constraint/linear.hpp"
#include "analyzer/core/domain/demo_dom.hpp"
#include "analyzer/core/domain/domains.hpp"
#include "analyzer/core/domain/numerical/interval_dom.hpp"
#include "analyzer/core/program_state.hpp"
#include "analyzer/core/region/region.hpp"
#include "analyzer/tooling/context.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <llvm/Support/raw_ostream.h>

#include <utility>
#include "common/util/log.hpp"

namespace knight::analyzer {

constexpr unsigned EventHandlerAlignment = 16U;

class NumericalAnalysis
    : public Analysis<
          NumericalAnalysis,
          analyze::EventListener< LinearNumericalAssignEvent,
                                  LinearNumericalAssumptionEvent > > {
  public:
    explicit NumericalAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::NumericalAnalysis;
    }

    struct alignas(EventHandlerAlignment) LinearNumericalAssignEventHandler {
        const NumericalAnalysis& analysis;
        ProgramStateRef& state;

        LinearNumericalAssignEventHandler(const NumericalAnalysis& analysis,
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

    }; // struct EventHandler

    void handle_event(LinearNumericalAssignEvent* event) const {
        std::visit(LinearNumericalAssignEventHandler{*this, event->state},
                   event->assign);
    }

    struct LinearNumericalAssumptionEventHandler {
        const NumericalAnalysis& analysis;
        ProgramStateRef& state;

        LinearNumericalAssumptionEventHandler(const NumericalAnalysis& analysis,
                                              ProgramStateRef& state)
            : analysis(analysis), state(state) {}

        template < typename T >
        void operator()(const T& event) const {
            this->handle(event);
        }

        void handle(const PredicateZVarZNum& pred) const;
        void handle(const PredicateZVarZVar& pred) const;
        void handle(const GeneralLinearConstraint& cstr) const;

    } __attribute__((aligned(
        EventHandlerAlignment))); // struct
                                  // LinearNumericalAssumptionEventHandler

    void handle_event(LinearNumericalAssumptionEvent* event) const {
        std::visit(LinearNumericalAssumptionEventHandler{*this, event->state},
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

} // namespace knight::analyzer