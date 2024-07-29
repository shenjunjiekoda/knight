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

#include "dfa/analysis_context.hpp"
#include "dfa/analysis_manager.hpp"
#include "dfa/analysis/analyses.hpp"
#include "support/clang_ast.hpp"

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
///        (more callbacks accpeted...) > {
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
///        return mgr.register_analysis< MyAnalysisImpl >(ctx, (more ctor args...));
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
    virtual bool is_language_supported(
        const clang::LangOptions& lang_opts) const {
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
    Analysis< Impl, ANALYSIS1, ANALYSES... >(KnightContext& ctx)
        : AnalysisBase(ctx, Impl::get_kind()) {}

    static void register_callback(Impl* checker, AnalysisManager& mgr) {
        ANALYSIS1::register_callback(checker, mgr);
        Analysis< Impl, ANALYSES... >::register_callback(checker, mgr);
    }
};

template < typename Impl, typename ANALYSIS1 >
class Analysis< Impl, ANALYSIS1 > : public ANALYSIS1, public AnalysisBase {
  public:
    Analysis< ANALYSIS1 >(KnightContext& ctx)
        : AnalysisBase(ctx, Impl::get_kind()) {}

    static void register_callback(Impl* checker, AnalysisManager& mgr) {
        ANALYSIS1::register_callback(checker, mgr);
    }
};

//=------------------- Callback Registration -------------------=//

namespace analyze {

class BeginFunction {
    template < typename ANALYSIS >
    static void run_begin_function(void* analysis, AnalysisContext& C) {
        ((const ANALYSIS*)analysis)->analyze_begin_function(C);
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_begin_function(
            internal::AnalyzeBeginFunctionCallBack(analysis,
                                                   run_begin_function<
                                                       ANALYSIS >));
    }
}; // class BeginFunction

class EndFunction {

    template < typename ANALYSIS >
    static void run_end_function(void* analysis, AnalysisContext& C) {
        ((const ANALYSIS*)analysis)->analyze_end_function(C);
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_end_function(
            internal::AnalyzeEndFunctionCallBack(analysis,
                                                 run_end_function< ANALYSIS >));
    }

}; // class EndFunction

template < clang_stmt STMT > class PreStmt {
    template < typename ANALYSIS >
    static void run_pre_stmt(void* analysis,
                             internal::StmtRef S,
                             AnalysisContext& C) {
        ((const ANALYSIS*)analysis)->pre_analyze_stmt(S, C);
    }

    static bool is_interesting_stmt(internal::StmtRef S) {
        return isa< STMT >(S);
    }

  public:
    template < typename ANALYSIS >
    void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_stmt(internal::AnalyzeStmtCallBack(analysis,
                                                            run_pre_stmt<
                                                                ANALYSIS >),
                              is_interesting_stmt,
                              internal::VisitStmtKind::Pre);
    }

}; // class PreStmt

template < clang_stmt STMT > class EvalStmt {
    template < typename ANALYSIS >
    static void run_eval_stmt(void* analysis,
                              internal::StmtRef S,
                              AnalysisContext& C) {
        ((const ANALYSIS*)analysis)->analyze_stmt(S, C);
    }

    static bool is_interesting_stmt(internal::StmtRef S) {
        return isa< STMT >(S);
    }

  public:
    template < typename ANALYSIS >
    void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_stmt(internal::AnalyzeStmtCallBack(analysis,
                                                            run_eval_stmt<
                                                                ANALYSIS >),
                              is_interesting_stmt,
                              internal::VisitStmtKind::Eval);
    }
}; // class EvalStmt

template < clang_stmt STMT > class PostStmt {
    template < typename ANALYSIS >
    static void run_post_stmt(void* analysis,
                              internal::StmtRef S,
                              AnalysisContext& C) {
        ((const ANALYSIS*)analysis)->post_analyze_stmt(S, C);
    }

    static bool is_interesting_stmt(internal::StmtRef S) {
        return isa< STMT >(S);
    }

  public:
    template < typename ANALYSIS >
    void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_stmt(internal::AnalyzeStmtCallBack(analysis,
                                                            run_post_stmt<
                                                                ANALYSIS >),
                              is_interesting_stmt,
                              internal::VisitStmtKind::Post);
    }
}; // class PostStmt

} // namespace analyze

} // namespace knight::dfa
