//===- analysis_base.hpp ----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the base for all data flow analyses.
//
//===------------------------------------------------------------------===//

#pragma once

#include "clang/AST/Stmt.h"
#include "util/log.hpp"

#include "dfa/analysis/analyses.hpp"
#include "dfa/analysis_context.hpp"
#include "dfa/analysis_manager.hpp"
#include "support/clang_ast.hpp"
#include "support/event.hpp"

namespace knight::dfa {

class AnalysisBase;

/// \brief Base class for all data flow analyses.
///
/// Derived class can inherit like this:
/// \code
/// class MyAnalysisImpl :
///    public Analysis<MyAnalysisImpl,
///        analyze::BeginFunction,
///        analyze::EndFunction,
///        (more callbacks accepted...) > {
///
///    /// \brief delegate to the Analysis constructor
///    MyAnalysisImpl(KnightContext& ctx, (more args...))
///         : Analysis(ctx) {
///       // ...
///    }
///
///    /// \brief tell us the kind of analysis.
///    static AnalysisKind get_kind() {
///        return AnalysisKind::MyAnalysis;
///    }
///
///    /// \brief Derived analysis can add analysis dependencies by
///    /// implementing the following function:
///    static void add_dependencies(AnalysisManager& mgr) {
///        // add dependencies here
///    }
///
///    /// \brief BeginFunction callback.
///    void analyze_begin_function(AnalysisContext& C) const {
///        // analyze begin function here.
///    }
///
///    /// \brief EndFunction callback.
///    void analyze_end_function(AnalysisContext& C) const {
///        // analyze end function here.
///    }
///
///    // add more analysis callbacks functions here.
///
///    /// \brief register your analysis to manager here.
///    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
///                                             KnightContext& ctx) {
///        return mgr.register_analysis< MyAnalysisImpl >(ctx, (more ctor
///        args...));
///    }
///
/// }; // class MyAnalysisImpl
/// \endcode
class AnalysisBase {
  public:
    AnalysisKind kind;

  public:
    AnalysisBase(KnightContext& ctx, AnalysisKind k);
    virtual ~AnalysisBase() = default;
    [[nodiscard]] virtual bool is_language_supported(
        [[maybe_unused]] const clang::LangOptions& lang_opts) const {
        return true;
    }

  protected:
    clang::DiagnosticBuilder diagnose(
        clang::SourceLocation loc,
        llvm::StringRef info,
        clang::DiagnosticIDs::Level diag_level = clang::DiagnosticIDs::Warning);

    clang::DiagnosticBuilder diagnose(
        llvm::StringRef info,
        clang::DiagnosticIDs::Level diag_level = clang::DiagnosticIDs::Warning);

    clang::DiagnosticBuilder diagnose(
        clang::DiagnosticIDs::Level diag_level = clang::DiagnosticIDs::Warning);

    KnightContext& get_knight_context() { return m_ctx; }

  private:
    KnightContext& m_ctx;
}; // class AnalysisBase

template < typename Impl, typename ANALYSIS1, typename... ANALYSES >
class Analysis : public ANALYSIS1, public ANALYSES..., public AnalysisBase {
  public:
    explicit Analysis(KnightContext& ctx)
        : AnalysisBase(ctx, Impl::get_kind()) {}

    static void register_callback(Impl* analysis, AnalysisManager& mgr) {
        ANALYSIS1::register_callback(analysis, mgr);
        Analysis< Impl, ANALYSES... >::register_callback(analysis, mgr);
    }
};

template < typename Impl, typename ANALYSIS1 >
class Analysis< Impl, ANALYSIS1 > : public ANALYSIS1, public AnalysisBase {
  public:
    explicit Analysis(KnightContext& ctx)
        : AnalysisBase(ctx, Impl::get_kind()) {}

    static void register_callback(Impl* analysis, AnalysisManager& mgr) {
        ANALYSIS1::register_callback(analysis, mgr);
    }
};

//=------------------- Callback Registration -------------------=//

namespace analyze {

class BeginFunction {
    template < typename ANALYSIS >
    static void run_begin_function(void* analysis, AnalysisContext& C) {
        (static_cast< const ANALYSIS* >(analysis))->analyze_begin_function(C);
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_begin_function(
            internal::AnalyzeBeginFunctionCallBack(ANALYSIS::get_kind(),
                                                   analysis,
                                                   run_begin_function<
                                                       ANALYSIS >));
    }
}; // class BeginFunction

class EndFunction {
    template < typename ANALYSIS >
    static void run_end_function(void* analysis, AnalysisContext& C) {
        (static_cast< const ANALYSIS* >(analysis))->analyze_end_function(C);
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_end_function(
            internal::AnalyzeEndFunctionCallBack(ANALYSIS::get_kind(),
                                                 analysis,
                                                 run_end_function< ANALYSIS >));
    }

}; // class EndFunction

template < clang_stmt STMT >
class PreStmt {
    template < typename ANALYSIS >
    static void run_pre_stmt(void* analysis,
                             internal::StmtRef stmt,
                             AnalysisContext& ctx) {
        (static_cast< const ANALYSIS* >(analysis))
            ->pre_analyze_stmt(dyn_cast< STMT >(stmt), ctx);
    }

    static bool is_interesting_stmt(internal::StmtRef stmt) {
        return isa< STMT >(stmt);
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_stmt(internal::
                                  AnalyzeStmtCallBack(ANALYSIS::get_kind(),
                                                      analysis,
                                                      run_pre_stmt< ANALYSIS >),
                              is_interesting_stmt,
                              internal::VisitStmtKind::Pre);
    }

}; // class PreStmt

template < clang_stmt STMT >
class EvalStmt {
    template < typename ANALYSIS >
    static void run_eval_stmt(void* analysis,
                              internal::StmtRef stmt,
                              AnalysisContext& ctx) {
        (static_cast< const ANALYSIS* >(analysis))
            ->analyze_stmt(dyn_cast< STMT >(stmt), ctx);
    }

    static bool is_interesting_stmt(internal::StmtRef stmt) {
        return isa< STMT >(stmt);
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_stmt(internal::
                                  AnalyzeStmtCallBack(ANALYSIS::get_kind(),
                                                      analysis,
                                                      run_eval_stmt<
                                                          ANALYSIS >),
                              is_interesting_stmt,
                              internal::VisitStmtKind::Eval);
    }
}; // class EvalStmt

template < clang_stmt STMT >
class PostStmt {
    template < typename ANALYSIS >
    static void run_post_stmt(void* analysis,
                              internal::StmtRef stmt,
                              AnalysisContext& ctx) {
        (static_cast< const ANALYSIS* >(analysis))
            ->post_analyze_stmt(dyn_cast< STMT >(stmt), ctx);
    }

    static bool is_interesting_stmt(internal::StmtRef stmt) {
        return isa< STMT >(stmt);
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_stmt(internal::
                                  AnalyzeStmtCallBack(ANALYSIS::get_kind(),
                                                      analysis,
                                                      run_post_stmt<
                                                          ANALYSIS >),
                              is_interesting_stmt,
                              internal::VisitStmtKind::Post);
    }
}; // class PostStmt

class ConditionFilter {
    template < typename ANALYSIS >
    static void filter_condition(void* analysis,
                                 internal::ExprRef expr,
                                 bool assertion_result,
                                 AnalysisContext& ctx) {
        (static_cast< const ANALYSIS* >(analysis))
            ->filter_condition(expr, assertion_result, ctx);
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_condition_filter(
            internal::ConditionFilterCallback(ANALYSIS::get_kind(), analysis));
    }

}; // class ConditionFilter

template < event EVENT >
class EventDispatcher {
  private:
    AnalysisManager* m_analysis_mgr_from_event{};

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_event_dispatcher< EVENT >();
        static_cast< EventDispatcher< EVENT >* >(analysis)
            ->m_analysis_mgr_from_event = &mgr;
    }

    void dispatch_event(EVENT& event) const {
        m_analysis_mgr_from_event->dispatch_event(event);
    }
};

template < event EVENT >
class EventListener {
    template < typename ANALYSIS >
    static void handle_event(void* analysis, void* event) {
        (static_cast< const ANALYSIS* >(analysis))
            ->handle_event(static_cast< EVENT* >(event));
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_event_listener< EVENT >(
            internal::EventListenerCallback(ANALYSIS::get_kind(),
                                            analysis,
                                            handle_event< ANALYSIS >));
    }
};

} // namespace analyze

} // namespace knight::dfa
